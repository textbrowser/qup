#!/usr/bin/env sh

# Alexis Megas.

export AA_ENABLEHIGHDPISCALING=1
export AA_USEHIGHDPIPIXMAPS=1
export QT_AUTO_SCREEN_SCALE_FACTOR=1
export QT_X11_NO_MITSHM=1

kde=$(env | grep -ci kde 2>/dev/null)

if [ $kde -gt 0 ]
then
    echo "KDE!"

    qup_arguments=style="-style=Breeze"
else
    qup_arguments=style="-style=Fusion"
fi

# Begin Qup
# Here be special Qup instructions.
# End Qup

if [ -r ./Qup ] && [ -x ./Qup ]
then
    echo "Launching a local Qup."

    if [ -r ./Lib ]
    then
	export LD_LIBRARY_PATH=Lib
    fi

    ./Qup "$style" "$@"
    exit $?
elif [ -r /opt/qup/Qup ] && [ -x /opt/qup/Qup ]
then
    echo "Launching an official Qup."
    cd /opt/qup && ./Qup "$style" "$@"
    exit $?
elif [ -r /usr/local/qup/Qup ] && [ -x /usr/local/qup/Qup ]
then
    echo "Launching an official Qup."
    cd /usr/local/qup && ./Qup "$style" "$@"
    exit $?
else
    echo "Cannot locate Qup. Why?"
    exit 1
fi
