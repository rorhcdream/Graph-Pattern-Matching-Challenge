#/bin/sh

mkdir ../output
for d in ../data/* ; do
	dname=$(echo $d | cut -d"/" -f 3 | cut -d"." -f 1)
	echo "data $dname"

	for q in ../query/* ; do
		qname=$(echo $q | cut -d"/" -f 3 | cut -d"." -f 1)
		if [[ "$qname" =~ "$dname" ]]; then
			echo "	query $qname"
			c="../candidate_set/$qname.cs"
			diff ../output/$qname.out ../output_copy/$qname.out
		fi
	done
done
