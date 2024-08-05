    // char encoded_str[MAX_STR_LEN] = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:'Debian CD from cdimage.debian.org'13:creation datei1573903810e4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:pieces3:ABCee";

    // int offset = b_get_in_list_offset(LIST, 0, encoded_str, strlen(encoded_str));
    // printf("%d %c\n", offset, encoded_str[offset]);

    // b_print(encoded_str, strlen(encoded_str)), 0, LIST;
    // printf("\n");

    // char obj[MAX_STR_LEN];
    // b_get("0.d|", encoded_str, strlen(encoded_str), obj);
    // printf("%s\n", obj);

    // char b_info_str[MAX_STR_LEN];
    // b_create_list(b_info_str);

    // b_insert_element(b_info_str, strlen(encoded_str), "0.l|", "length", OTHER);
    // b_insert_element(b_info_str, strlen(encoded_str), "0.l|", "name", OTHER);
    // b_insert_element(b_info_str, strlen(encoded_str), "0.l|", "AAA", OTHER);

    // char b_str[MAX_STR_LEN];
    // b_create_dict(b_str);

    // b_insert_key_value(b_str, strlen(encoded_str), "0.d|", "announce", "http", OTHER);
    // b_insert_key_value(b_str, strlen(encoded_str), "0.d|", "info", b_info_str, DICTIONARY);
    // b_insert_int(b_str, strlen(encoded_str), "0.d|info|0.l|", -17);
    
    // b_print(b_str, strlen(b_str));    

    // printf("%s", b_str);