#!/bin/sh
printf "comment_t comments[]= {\n" > comments_generated.h
cat Decode.txt | awk '
	/0x[0-9a-f][0-9a-f] / {
		sub(/0x/,"0x1",$1);
		cmt=substr($0,8);
		sub(/^[[:space:]]*/,"",cmt);
		print "\t{"$1 ", \"" cmt "\"},";
	};
' >> comments_generated.h
cat Comments.txt | awk '
	BEGIN {FS="\t";};
	/0x[0-9a-f][0-9a-f][0-9a-f]/ {
		print "\t{"$1 ", \"" $2 "\"},";
	};
' >> comments_generated.h
printf "\t{0,0}\n};" >> comments_generated.h
