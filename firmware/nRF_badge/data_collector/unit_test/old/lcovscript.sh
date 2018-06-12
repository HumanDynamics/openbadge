#filename="/app/firmware/nRF_badge/data_collector/unit_test/_build/LCOV/run_eeprom_lib_mock_unittest/coverage.info"
filename="coverage.info"
#exec 4<$filename

cur_src_file=""
while read line; do 
	case "$line" in 
	*SF:*)
#splitting the line by ":"
	cur_src_file="$(cut -d':' -f2 <<<"$line")";;
	*FNH:0*)
#adding the zero call file to array
	zero_call_src_files+=$cur_src_file
	zero_call_src_files+=" "
	cur_src_file="";
	esac
done < $filename

echo Zero call src files: $zero_call_src_files
	lcov --remove coverage.info $zero_call_src_files -o coverage.info