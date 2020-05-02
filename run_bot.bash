# Skript zum Testen des Bots:
# Startet den Bot auf Datei testSites_10_2019.txt, 
# Delay fuer Zugriff auf Seite wird auf 100 ms eingestellt, 
# gelesene Daten werden in Verzeichnis 'download' geschrieben. 
#

execute(){

./bot $1 --webreq-delay 0 --webreq-path $2/$3_0ms $4
./bot $1 --webreq-delay 300 --webreq-path $2/$3_300ms $4
./bot $1 --webreq-delay 500 --webreq-path $2/$3_500ms $4
./bot $1 --webreq-delay 1000 --webreq-path $2/$3_1000ms $4

}

TIMEFILE="elapsed_time.txt"
DOWNLOADDIR="downloads"

echo " Please enter the File you want to read the internet sides from "

read SITESFILE

if [ ! -f $SITESFILE ]; then
	
	echo "$SITESFILE doesnt exists"
	echo
	exit
fi

if [ ! -f $TIMEFILE ]; then
	
	touch $TIMEFILE
	echo
else
	echo "" > $TIMEFILE	
fi

if [ ! -d $DOWNLOADDIR ]; then

	mkdir $DOWNLOADDIR
else
	cd $DOWNLOADDIR
	rm -r *ms
	cd ..
fi

echo "Please enter the number of times you want to run the test.."

read CHANGENUMBER

COUNTER=0

while [ "$COUNTER" -ne "$CHANGENUMBER" ]; do
	
	echo	
	echo "Please set the number of clients at the beginning of bot.cpp"
      	echo "Compile the file and enter the amount of clients to run the test..."
 	read CLIENTAMOUNT
	
	echo "" >> $TIMEFILE
	execute $SITESFILE $DOWNLOADDIR $CLIENTAMOUNT $TIMEFILE
	((COUNTER++))	

done	

echo "Test finished..."
exit
