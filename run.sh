tar -xf linux.tar.gz
tar -xf mysql.tar.gz
tar -xf ssl.tar.gz
tar -xf nfs.tar.gz

rm -rf result
rm -rf tmp
python -m venv vc-env
source vc-env/bin/activate
mkdir -p result
mkdir -p tmp
HOME_DIR=`pwd`

touch result/table_2_detected_bugs.csv
echo "Application, Detected" >> result/table_2_detected_bugs.csv
touch result/table_7_time_analysis.csv
echo "Application, Time" >> result/table_7_time_analysis.csv
touch result/table_6_dok_effect.csv
echo "Application, DOK, DOK-AC, DOK-DL, DOK-FA" >> result/table_6_dok_effect.csv

START=$(date +%s)
./analysis.sh nfs
python valuecheck/main.py  /home/lily/HAPPY/LAB/apiusage/nfs-ganesha-new/ $HOME_DIR/nfs-ganesha/ tmp/nfs-liveness nfs > tmp/nfs-no-author
python valuecheck/check_author.py tmp/nfs-no-author $HOME_DIR/nfs-ganesha/ nfs > tmp/nfs-with-author
python valuecheck/dok_rank.py tmp/nfs-with-author 17 >> result/table_6_dok_effect.csv
END=$(date +%s)
DIFF=$(( $END - $START ))
echo "NFS-ganesha, $DIFF" >> result/table_7_time_analysis.csv

START=$(date +%s)
./analysis.sh linux
python valuecheck/main.py  /home/lily/HAPPY/LAB/apiusage/linux/ $HOME_DIR/linux/ tmp/linux-liveness linux > tmp/linux-no-author
python valuecheck/check_author.py tmp/linux-no-author $HOME_DIR/linux/ linux > tmp/linux-with-author
python valuecheck/dok_rank.py tmp/linux-with-author 20 >> result/table_6_dok_effect.csv
END=$(date +%s)
DIFF=$(( $END - $START ))
echo "Linux, $DIFF" >> result/table_7_time_analysis.csv

START=$(date +%s)
./analysis.sh mysql
python valuecheck/main.py  /home/lily/HAPPY/LAB/apiusage/mysql-server-new/ $HOME_DIR/mysql-server/ tmp/mysql-liveness mysql > tmp/mysql-no-author
python valuecheck/check_author.py tmp/mysql-no-author $HOME_DIR/mysql-server/ mysql > tmp/mysql-with-author
python valuecheck/dok_rank.py tmp/mysql-with-author 20 >> result/table_6_dok_effect.csv
END=$(date +%s)
DIFF=$(( $END - $START ))
echo "MySQL, $DIFF" >> result/table_7_time_analysis.csv

START=$(date +%s)
./analysis.sh ssl
python valuecheck/main.py  /home/lily/HAPPY/LAB/apiusage/openssl-new/ $HOME_DIR/openssl/ tmp/ssl-liveness ssl > tmp/ssl-no-author
python valuecheck/check_author.py tmp/ssl-no-author $HOME_DIR/openssl/ ssl > tmp/ssl-with-author
python valuecheck/dok_rank.py tmp/ssl-with-author 17 >> result/table_6_dok_effect.csv
END=$(date +%s)
DIFF=$(( $END - $START ))
echo "OpenSSL, $DIFF" >> result/table_7_time_analysis.csv

rm -rf tmp/

python valuecheck/figure.py
