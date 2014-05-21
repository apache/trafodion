/<unistd.h>/ { print "// ",$0; next; }
			{ gsub(/yy/,arkstr); print $0; }
