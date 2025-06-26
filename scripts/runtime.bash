: "${RUNTIME_arch_run?}"
: "${MAX_SMP:=$(getconf _NPROCESSORS_ONLN)}"
: "${TIMEOUT:=90s}"

PASS() { echo -ne "\e[32mPASS\e[0m"; }
SKIP() { echo -ne "\e[33mSKIP\e[0m"; }
FAIL() { echo -ne "\e[31mFAIL\e[0m"; }

extract_summary()
{
    local cr=$'\r'
    tail -5 | grep '^SUMMARY: ' | sed 's/^SUMMARY: /(/;s/'"$cr"'\{0,1\}$/)/'
}

# We assume that QEMU is going to work if it tried to load the kernel
premature_failure()
{
    local log

    log="$(eval "$(get_cmdline _NO_FILE_4Uhere_)" 2>&1)"

    echo "$log" | grep "_NO_FILE_4Uhere_" |
        grep -q -e "[Cc]ould not \(load\|open\) kernel" \
                -e "error loading" \
                -e "failed to load" &&
        return 1

    RUNTIME_log_stderr <<< "$log"

    echo "$log"
    return 0
}

get_cmdline()
{
    local kernel=$1
    echo "TESTNAME=$testname TIMEOUT=$timeout MACHINE=$machine ACCEL=$accel $RUNTIME_arch_run $kernel $smp $test_args $opts"
}

skip_nodefault()
{
    [ "$run_all_tests" = "yes" ] && return 1
    [ "$KUT_STANDALONE" != "yes" ] && return 0

    while true; do
        read -r -p "Test marked not to be run by default, are you sure (y/N)? " yn
        case $yn in
            "Y" | "y" | "Yes" | "yes")
                return 1
                ;;
            "" | "N" | "n" | "No" | "no" | "q" | "quit" | "exit")
                return 0
                ;;
        esac
    done
}

function print_result()
{
    local status="$1"
    local testname="$2"
    local summary="$3"
    local reason="$4"

    if [ -z "$reason" ]; then
        echo "$($status) $testname $summary"
    else
        echo "$($status) $testname ($reason)"
    fi
}

function find_word()
{
    grep -Fq " $1 " <<< " $2 "
}

function run()
{
    local testname="$1"
    local groups="$2"
    local smp="$3"
    local kernel="$4"
    local test_args="$5"
    local opts="$6"
    local arch="$7"
    local machine="$8"
    local check="${CHECK:-$9}"
    local accel="${10}"
    local timeout="${11:-$TIMEOUT}" # unittests.cfg overrides the default

    if [ "${CONFIG_EFI}" == "y" ]; then
        kernel=${kernel/%.flat/.efi}
    fi

    if [ -z "$testname" ]; then
        return
    fi

    if [ -n "$only_tests" ] && ! find_word "$testname" "$only_tests"; then
        return
    fi

    if [ -n "$only_group" ] && ! find_word "$only_group" "$groups"; then
        return
    fi

    if [ -z "$GEN_SE_HEADER" ] && find_word "pv-host" "$groups"; then
        print_result "SKIP" $testname "" "no gen-se-header available for pv-host test"
        return
    fi

    if [ -z "$only_group" ] && find_word nodefault "$groups" &&
            skip_nodefault; then
        print_result "SKIP" $testname "" "test marked as manual run only"
        return;
    fi

    if [ -n "$arch" ] && [ "$arch" != "$ARCH" ]; then
        print_result "SKIP" $testname "" "$arch only"
        return 2
    fi

    if [ -n "$machine" ] && [ -n "$MACHINE" ] && [ "$machine" != "$MACHINE" ]; then
        print_result "SKIP" $testname "" "$machine only"
        return 2
    elif [ -n "$MACHINE" ]; then
        machine="$MACHINE"
    fi

    if [ -n "$accel" ] && [ -n "$ACCEL" ] && [[ ! "$ACCEL" =~ ^$accel(,|$) ]]; then
        print_result "SKIP" $testname "" "$accel only, but ACCEL=$ACCEL"
        return 2
    elif [ -n "$ACCEL" ]; then
        accel="$ACCEL"
    fi

    # check a file for a particular value before running a test
    # the check line can contain multiple files to check separated by a space
    # but each check parameter needs to be of the form <path>=<value>
    if [ "$check" ]; then
        # There is no globbing or whitespace allowed in check parameters.
        # shellcheck disable=SC2206
        check=($check)
        for check_param in "${check[@]}"; do
            path=${check_param%%=*}
            value=${check_param#*=}
            if ! [ -f "$path" ] || [ "$(cat $path)" != "$value" ]; then
                print_result "SKIP" $testname "" "$path not equal to $value"
                return 2
            fi
        done
    fi

    log=$(premature_failure) && {
        skip=true
        if [ "${CONFIG_EFI}" == "y" ]; then
            if [ "$ARCH" == "x86_64" ] &&
               [[ "$(tail -1 <<<"$log")" =~ "Dummy Hello World!" ]]; then
                   skip=false
            elif [ "$ARCH" == "arm64" ] &&
               [[ "$(tail -2 <<<"$log" | head -1)" =~ "Dummy Hello World!" ]]; then
                   skip=false
            fi
        fi

        if [ ${skip} == true ]; then
            print_result "SKIP" $testname "" "$(tail -1 <<<"$log")"
            return 77
        fi
    }

    cmdline=$(get_cmdline $kernel)
    if find_word "migration" "$groups"; then
        cmdline="MIGRATION=yes $cmdline"
    fi
    if find_word "panic" "$groups"; then
        cmdline="PANIC=yes $cmdline"
    fi
    if [ "$verbose" = "yes" ]; then
        echo $cmdline
    fi

    # qemu_params/extra_params in the config file may contain backticks that
    # need to be expanded, so use eval to start qemu.  Use "> >(foo)" instead of
    # a pipe to preserve the exit status.
    summary=$(eval "$cmdline" 2> >(RUNTIME_log_stderr $testname) \
                             > >(tee >(RUNTIME_log_stdout $testname $kernel) | extract_summary))
    ret=$?
    [ "$KUT_STANDALONE" != "yes" ] && echo > >(RUNTIME_log_stdout $testname $kernel)

    if [ $ret -eq 0 ]; then
        print_result "PASS" $testname "$summary"
    elif [ $ret -eq 77 ]; then
        print_result "SKIP" $testname "$summary"
    elif [ $ret -eq 124 ]; then
        print_result "FAIL" $testname "" "timeout; duration=$timeout"
        if [ "$tap_output" = "yes" ]; then
            echo "not ok TEST_NUMBER - ${testname}: timeout; duration=$timeout" >&3
        fi
    elif [ $ret -gt 127 ]; then
        signame="SIG"$(kill -l $(($ret - 128)))
        print_result "FAIL" $testname "" "terminated on $signame"
        if [ "$tap_output" = "yes" ]; then
            echo "not ok TEST_NUMBER - ${testname}: terminated on $signame" >&3
        fi
    elif [ $ret -eq 127 ] && [ "$tap_output" = "yes" ]; then
        echo "not ok TEST_NUMBER - ${testname}: aborted" >&3
    else
        print_result "FAIL" $testname "$summary"
    fi

    return $ret
}

#
# Probe for MAX_SMP, in case it's less than the number of host cpus.
#
function probe_maxsmp()
{
	local smp

	if smp=$($RUNTIME_arch_run _NO_FILE_4Uhere_ -smp $MAX_SMP |& grep 'SMP CPUs'); then
		smp=${smp##* }
		smp=${smp/\(}
		smp=${smp/\)}
		echo "Restricting MAX_SMP from ($MAX_SMP) to the max supported ($smp)" >&2
		MAX_SMP=$smp
	fi
}
