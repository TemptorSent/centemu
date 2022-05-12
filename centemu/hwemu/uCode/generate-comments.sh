#!/bin/sh
printf "comment_t comments[]= {\n" > comments_generated.h
cat Comments/Decode.txt | awk '
	/0x[0-9a-f][0-9a-f] / {
		sub(/0x/,"0x1",$1);
		cmt=substr($0,8);
		sub(/^[[:space:]]*/,"",cmt);
		print "\t{"$1 ", \"MAPROM Entry Point: " cmt "\"},";
	};
' >> comments_generated.h
cat Comments/Comments.txt | awk '
	BEGIN {FS="\t";};
	/0x[0-9a-f][0-9a-f][0-9a-f]/ {
		print "\t{"$1 ", \"" $2 "\"},";
	};
' >> comments_generated.h

cat Comments/sjsoftware/bootstrap.txt | awk '
	/\([0-9a-f][0-9a-f][0-9a-f]\)[[:space:]]+;/ {
		addr=substr($0,132,3);
		cmt=substr($0,139);
		print "\t{0x" addr ", \"sjsoftware: " cmt "\"},";
	};
' >> comments_generated.h
printf "\t{0,0}\n};" >> comments_generated.h
