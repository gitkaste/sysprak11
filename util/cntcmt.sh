#!/bin/sh
## Count comments

function cWithout() {
	echo -n $(
		gcc -fpreprocessed -dD -E "$1" 2>/dev/null | \
		grep -vP "# \\d* \"$1\"" | \
		wc -m | \
		cut -f1 -d' '
	)
}

function cWith() {
	echo -n $(
		wc -m "$f" | \
		cut -f1 -d' '
	)
}

echo "Rat|File name                     |Letters incl. comments"
echo "===+==============================+======================"
(
	for f in "$@"
	do
		cWo=`cWithout "$f"`
		cW=`cWith "$f"`
		
		cRatio=$(( 100 - ( $cWo * 100 / $cW ) ))
		
		printf "%3i %-30s %i\n" $cRatio  "$f" $cW
	done
) | sort -nr