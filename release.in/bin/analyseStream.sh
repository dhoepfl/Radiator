#!  /bin/bash

SCRIPT_HOME="$(dirname "$0")"
[ -e "$HOME/.analysestreamrc" ] && source "$HOME/.analysestreamrc"
[ -e "$SCRIPT_HOME/analyseStream.conf" ] && source "$SCRIPT_HOME/analyseStream.conf"
[ -e "$SCRIPT_HOME/../etc/analyseStream.conf" ] && source "$SCRIPT_HOME/../etc/analyseStream.conf"

if [ -z "$MAIL_FROM" -o -z "$MAIL_TO" ] ; then
   echo "\$HOME/.analysestreamrc or $SCRIPT_HOME/analyseStream.conf must set" >&2
   echo "MAIL_FROM and MAIL_TO" >&2
   echo "" >&2
   echo "Example analyseStream.conf:" >&2
   echo "" >&2
   echo "   MAIL_FROM='<heizung@example.com>'" >&2
   echo "   MAIL_TO='<janitor@example.com>'" >&2
   echo "" >&2
   exit 1
fi

OLDSTATUS=""
TIMESTAMP=""
while read X; do
   if [[ "$X" =~ ^\[TIME\]\ [A-Za-z]*,\ ([-0-9]*),\ ([:0-9]*) ]] ; then
      TIMESTAMP="${BASH_REMATCH[1]} ${BASH_REMATCH[2]}"
   fi

   if [ -n "$TIMESTAMP" ] ; then
      if [[ "$X" =~ ^\[VALUE\]\ 0*([0-9a-f]+)\ \[([^]]*)\]\ =\ \[([^]]*)\]\ \[(.)\] ]] ; then
         KEY="${BASH_REMATCH[1]}"
         NAME="${BASH_REMATCH[2]}"
         VALUE="${BASH_REMATCH[3]}"
         TYPE="${BASH_REMATCH[4]}"

         PRINTVALUES[$KEY]="$VALUE"
         RAWVALUES[$KEY]="$X"

         if [ "$TYPE" = "N" ] ; then
            VALUES[$KEY]="${VALUE//[^-0-9.]*/}"
         else
            VALUES[$KEY]="'$VALUE'"
         fi
      fi

      if [[ "$X" =~ ^$ ]] ; then
         echo -n "INSERT INTO Measurement VALUES('$TIMESTAMP'"
         for i in {0..29}; do
            echo -n ", ${VALUES[$i]}"
         done
         echo ");"

         if [ -n "$OLDSTATUS" ] ; then
            if [ "$OLDSTATUS" -ne "${PRINTVALUES[2]}" ] ; then
               (
               echo "From: $MAIL_FROM"
               echo "To: $MAIL_TO"
               echo "Subject: Statuswechsel Heizung"
               echo "Date: $(date -R)"
               echo "Content-Transfer-Encoding: 8bit"
               echo "Content-Type: text/plain; charset=\"UTF-8\""
               echo ""
               echo "Hallo,"
               echo ""
               echo "Der Status der Heizung hat sich gerade von $OLDSTATUS auf ${PRINTVALUES[2]} geändert." | iconv -f utf-8 -t latin1
               echo ""

               DISPLAY1=$(echo "${RAWVALUES[0]}" | sed -e 's/.*= .//' -e 's/\] \[[SON]\] *([0-9]*) *$//' | iconv -f utf-8 -t latin1)
               DISPLAY2=$(echo "${RAWVALUES[1]}" | sed -e 's/.*= .//' -e 's/\] \[[SON]\] *([0-9]*) *$//' | iconv -f utf-8 -t latin1)
               echo "             .-----------------,"
               printf "             | %15s |\n" "$DISPLAY1"
               printf "             | %15s |\n" "$DISPLAY2"
               echo "             \`-----------------'"
               echo ""
               echo "Aktuelle Messwerte:"
               echo ""
               for i in {2..29}; do
                  V=$(echo "${RAWVALUES[$i]}" | sed -e 's/^.VALUE. .. //' -e 's/.\[[SON]\] ([0-9]*)$//')
                  LABEL=$(echo "$V" | cut -d = -f 1 | sed -e 's/^.//' -e 's/. *$//' | iconv -f utf-8 -t latin1)
                  VALUE=$(echo "$V" | cut -d = -f 2 | sed -e 's/^ *.//' -e 's/. *$//' | iconv -f utf-8 -t latin1)
                  printf "%25s %6s\n" "$LABEL" "$VALUE"
               done
               ) | iconv -f latin1 -t utf-8 | sendmail "$MAIL_TO" >/dev/null 2>&1
            fi
         fi
         OLDSTATUS="${PRINTVALUES[2]}"
         unset VALUES
         unset PRINTVALUES
         unset RAWVALUES
      fi
   fi
done
