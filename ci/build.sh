#!/bin/sh
# Compiles and runs every C program under Chapter01..Chapter15.
# Collects every failure (does not stop at the first). Failures are printed as a
# workflow annotation, so they can be read without signing in to GitHub.
#
#   CC=gcc sh ci/build.sh
#
# Files whose first line says "RUN: compile only" are only compiled.
set -u
CC="${CC:-gcc}"
CFLAGS="-std=gnu11 -Wall -Wextra -O2 -pthread"
[ "$CC" = "gcc" ] && CFLAGS="$CFLAGS -Werror"
LIBS="-lm -lrt"
summary="${GITHUB_STEP_SUMMARY:-/dev/stdout}"
label="${RUNNER_OS:-local}/$CC"
root=$(pwd)
tmp=$(mktemp -d)
failed=""
ok=0

printf '\n## C programs — %s\n\n| File | Result |\n|---|---|\n' "$label" >> "$summary"

for f in Chapter*/*.c; do
    [ -f "$f" ] || continue
    name=$(echo "$f" | tr '/.' '__')
    exe="$tmp/$name"
    if ! $CC $CFLAGS "$f" -o "$exe" $LIBS > "$tmp/$name.log" 2>&1; then
        failed="$failed $f"
        printf '| `%s` | **DID NOT COMPILE** |\n' "$f" >> "$summary"
        continue
    fi
    if head -1 "$f" | grep -q 'RUN: compile only'; then
        ok=$((ok + 1))
        printf '| `%s` | compiled |\n' "$f" >> "$summary"
        continue
    fi
    run="$tmp/run_$name"
    mkdir -p "$run"
    if (cd "$run" && timeout 20 "$exe" < /dev/null > "$tmp/$name.out" 2>&1); then
        ok=$((ok + 1))
        printf '| `%s` | ok |\n' "$f" >> "$summary"
    else
        rc=$?
        { echo "exit code $rc"; tail -20 "$tmp/$name.out"; } >> "$tmp/$name.log"
        failed="$failed $f"
        printf '| `%s` | **FAILED (%s)** |\n' "$f" "$rc" >> "$summary"
    fi
    leftover=$(ls -A "$run")
    if [ -n "$leftover" ]; then
        failed="$failed $f"
        echo "files left behind: $leftover" >> "$tmp/$name.log"
        printf '| `%s` | **LEFT FILES** |\n' "$f" >> "$summary"
    fi
done

[ -z "$failed" ] && { printf '\n%s programs all passed on %s.\n' "$ok" "$label"; exit 0; }

printf '\n### Failures\n' >> "$summary"
for f in $failed; do
    name=$(echo "$f" | tr '/.' '__')
    printf '\n<details><summary>%s</summary>\n\n```\n' "$f" >> "$summary"
    head -40 "$tmp/$name.log" >> "$summary"
    printf '```\n\n</details>\n' >> "$summary"
done

# Collect the failures in one annotation (line breaks encoded as %0A).
msg=$(for f in $failed; do
        name=$(echo "$f" | tr '/.' '__')
        printf '%s:\n' "$f"
        grep -E 'error|warning|exit code|files left behind|undefined reference' "$tmp/$name.log" \
            | head -4 | sed 's/^/  /'
      done | sed 's/%/%25/g' | awk '{printf "%s%%0A", $0}')
printf '::error title=Chapter programs failed on %s::%s\n' "$label" "$msg"

printf '\nFailed:%s\n' "$failed"
cd "$root"
exit 1
