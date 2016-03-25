BEGIN {
    lv_header_line_number = 0;
    lv_lines_since_header = 0;
    lv_first = 1;
    lv_got_data = 0;
} 

{
    if (lv_header_line_number > 0) {
	lv_lines_since_header = FNR - lv_header_line_number;
	if (lv_lines_since_header >= 3) {
	    if ( ((lv_lines_since_header - 1) % 2) == 0) {
		lv_node_status = toupper($3);
		if (lv_node_status ~ "UP") {
		    lv_node_name = $8;
		}
		else {
		    lv_node_name = $4;
		}
		if (jason_output == 1) {
		    if (lv_first == 1) { 
			lv_first = 0;
			printf("[");
			lv_got_data = 1;
		    }
		    else {
			printf(",");
		    }
		    printf("{\"NODE\":\"%s\",\"STATUS\":\"%s\"}",
			   lv_node_name,
			   lv_node_status);
		}
		else {
		    printf ("%s [ %s ]\n",
			    lv_node_name,
			    lv_node_status);
		}
	    }
	}
    }
}

/MemFree SwapFree/ {lv_header_line_number = FNR;} 

END {
    if ((jason_output == 1) && 
	(lv_got_data == 1)) {
	printf("]\n");
    }
}
