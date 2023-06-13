#!/bin/bash

find_file() {
	if [ -d "$1" ]; then
		for i in "$1"/*; do
			find_file "$i"
		done
	elif [[ -f "$1" && ("$1" == *.c || "$1" == *.java || "$1" == *.py) ]]; then
		echo "$1"
	fi
}

#file_path,  foldername, input_folder
execute_code() {

	filename=$1
	foldername=$2
	test=$3

	test_cases=$(ls -1q $test | wc -l)

	if [[ "$filename" == *.c ]]; then
		gcc "$filename" -o "$foldername/main.out"
		for ((i = 1; i <= $test_cases; i++)); do
			"./$foldername/main.out" <"$test/test$i.txt" >"$foldername/out$i.txt"

		done
	elif [[ "$filename" == *.py ]]; then
		for ((i = 1; i <= $test_cases; i++)); do
			python3 "$filename" <"$test/test$i.txt" >"$foldername/out$i.txt"
		done

	elif [[ "$filename" == *.java ]]; then
		javac "$filename"
		for ((i = 1; i <= $test_cases; i++)); do
			java -cp "$foldername" Main <"$test/test$i.txt" >"$foldername/out$i.txt"
		done
	fi
}

# ================== Check flags ==================
verbosa_mode=false
execute_mode=true

#iterate the arguments and check if -noexe is present
for var in "$@"; do
	if [[ "$var" == "-noexecute" ]]; then
		execute_mode=false
	elif [[ "$var" == "-v" ]]; then
		verbosa_mode=true
	fi
done
# ================== Check flags ==================

submission_folder=$1
target_folder=$2
test_folder=$3
answer_folder=$4
test_cases=0

#create a temp folder
rm -rf temp_folder
rm -rf "$target_folder/result.csv"
#rm -rf "$2"

mkdir temp_folder
mkdir -p "$target_folder"
if [ "$execute_mode" = true ]; then
	touch "$target_folder/result.csv"
	echo "student_id,type,matched,not_matched" > "$target_folder/result.csv"
	test_cases=$(ls -1q "$test_folder" | wc -l)
fi


for i in "$submission_folder"/*; do
	student_id=${i:(-11):7}

	if [ "$verbosa_mode" = true ]; then
		echo "Organizing files of $student_id"
	fi

	unzip "$i" -d "temp_folder/$student_id" >/dev/null

	file_path=$(find_file "temp_folder/$student_id")
	dest_folder=""
	dest_file=""
	language=""

	if [[ "$file_path" == *.c ]]; then
		dest_folder="$target_folder/C/$student_id"
		dest_file="$target_folder/C/$student_id/main.c"
		language="C"
	elif [[ "$file_path" == *.java ]]; then
		dest_folder="$target_folder/Java/$student_id"
		dest_file="$target_folder/Java/$student_id/Main.java"
		language="Java"
	else
		dest_folder="$target_folder/Python/$student_id"
		dest_file="$target_folder/Python/$student_id/main.py"
		language="Python"
	fi

	mkdir -p "$dest_folder"
	mv "$file_path" "$dest_file"

	#======================== Execute Code ===================
	if [ "$execute_mode" = true ]; then
		if [ "$verbosa_mode" = true ]; then
			echo "Executing files of $student_id"
		fi

		execute_code "$dest_file" "$dest_folder" "$test_folder"

		execute_code "$dest_file" "$dest_folder" "$test_folder"

		difference=0

		for ((i = 1; i <= test_cases; i++)); do
			differ=$(diff -q "$dest_folder/out$i.txt" "$answer_folder/ans$i.txt" | wc -l)
			difference=$(($difference + $differ))
		done

		
		echo "$student_id,$language,$(($test_cases - $difference)),$difference" >> "$target_folder/result.csv"
	fi

done

rm -rf temp_folder
