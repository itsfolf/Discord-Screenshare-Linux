#!/bin/sh
username=$(logname)
dirs=(/home/$username/.config/discord*/*/modules/discord_voice /home/$username/.var/app/com.discordapp.*/config/discord/*/modules/discord_voice)
dirs=( $( for i in ${dirs[@]} ; do echo $i ; done | grep -v "*" ) )
len=${#dirs[@]}

if [[ $len -eq 0 ]]; then
    echo "No Discord installation found"
    exit 1
fi

if [[ $len -gt 1 ]]; then
    echo "Multiple Discord installations found"
    echo "Please select one:"
    for i in "${!dirs[@]}"; do
        echo -n "$((i+1)): "
        echo "${dirs[$i]}"
    done

    read -p "Enter selection: " selection
    selectedIndex=$((selection-1))
    selected=${dirs[$selectedIndex]}
    if [[ $selectedIndex -lt 0 || $selectedIndex -ge $len ]]; then
        echo "Invalid selection"
        exit 1
    fi
else
    selected=${dirs[0]}
fi

echo "Installing LinuxFix to $selected..."

chmod 644 $selected/index.js

separator="\/\/ ==Discord Screenshare Linux Fix=="

# Remove any existing installation
sed -i.bak '/^'"$separator"'/,/^'"$separator"'/{/^#/!{/^\$/!d}}' $selected/index.js

#Download latest
downloadUrl=$(wget -qO- https://api.github.com/repos/fuwwy/Discord-Screenshare-Linux/releases/latest | grep browser_download_url | cut -d '"' -f 4)
wget -O $selected/linux-fix.node -q --show-progress "$downloadUrl"
code=$(wget -qO- https://raw.githubusercontent.com/fuwwy/Discord-Screenshare-Linux/main/scripts/patch.js)

#echo -e "\n" >> $selected/index.js
echo "$code" >> $selected/index.js

chmod 444 $selected/index.js
