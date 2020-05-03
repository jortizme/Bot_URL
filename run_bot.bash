# Skript zum Testen des Bots: 
# gelesene Daten werden in Verzeichnis 'download/$(Anzahl-Treads)/$(Delayin-ms)' geschrieben. 
#

execute(){

./bot $1 --webreq-delay 0 --webreq-path $2/$3/0ms $4
./bot $1 --webreq-delay 300 --webreq-path $2/$3/300ms $4
./bot $1 --webreq-delay 500 --webreq-path $2/$3/500ms $4
./bot $1 --webreq-delay 1000 --webreq-path $2/$3/1000ms $4

}

TIMEFILE="elapsed_time.txt"
DOWNLOADDIR="downloads"

echo " Please enter the file you want to read the internet sides from "

read SITESFILE
echo

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
	rm -r $DOWNLOADDIR
	mkdir $DOWNLOADDIR
fi

echo "A single test runs the program with the set amount of clients"
echo "varying the proxy delay (0,300,500 and 1000 ms). To look at the "
echo "results open the $TIMEFILE file"
echo
echo "How many times do you want to run the test?"

read CHANGENUMBER

COUNTER=0

while [ "$COUNTER" -ne "$CHANGENUMBER" ]; do
	
	echo	
	echo "Please change the amount of clients at the beginning of bot.cpp"
      	echo "Compile the file and press [ENTER]"
	read
	echo "Enter the amount of clients set..."

 	read CLIENTAMOUNT
	
	echo "" >> $TIMEFILE
	execute $SITESFILE $DOWNLOADDIR $CLIENTAMOUNT $TIMEFILE
	((COUNTER++))	

done

echo "Test finished..."
exit
