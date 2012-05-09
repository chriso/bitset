#!/bin/sh
echo "# bitset changelog"
echo
git for-each-ref --sort='-authordate' --format='%(refname:short) %(*authordate)' refs/tags | while read tag; do
	if [ -z "$last_tag" ]; then
		last_tag=$tag
		continue
	fi

	date=`git log -1 --date=short --pretty="format:%ad" $tag`
	echo "## $last_tag ($date)"
	git log --format="%s [%an]" $tag..$last_tag | while read line; do
		echo " - $line"
	done

	last_tag=$tag
	echo
done
