#!/bin/sh



files=`find . -name "*.cpp"`
for i in $files; do
    cat $i | awk '{if ($0 ~ /TRANSLATE_MODULE\(/) { print $1" "$2" 0, "$3 } else {print $0} }' > $i.bak
    mv $i.bak $i
done
