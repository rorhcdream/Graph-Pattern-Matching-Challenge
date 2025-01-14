#/bin/sh

mkdir ../output
for d in ../data/* ; do
	dname=$(echo $d | cut -d"/" -f 3 | cut -d"." -f 1)
	echo "data $dname"

	for q in ../query/* ; do
		qname=$(echo $q | cut -d"/" -f 3 | cut -d"." -f 1)
		if [[ "$qname" =~ "$dname" ]]; then
			echo -n "	query $qname"
			c="../candidate_set/$qname.cs"
			trap "exit" INT
			timeout --foreground 60 ./main/program $d $q $c > ../output/$qname.out
			exit_status=$?
			echo " $(wc -l ../output/$qname.out | awk '{ print $1 }')"
			if [[ $exit_status -eq 124 ]]; then
			    echo "time out!"
			fi

		fi
	done
done
