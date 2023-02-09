# Discord Screenshare Linux
[Support Server](https://discord.gg/UKzXq7xkNU)

WIP repo that adds Discord screenshare support for Wayland and Pipewire aswell as audio sharing, without the need to use the web client.

**⚠️  Audio and XOrg support are currently WIP. At the moment all this does is make screenshare work on Wayland (no audio). Join the support server for updates/testing builds.**

## Install
```
sh -c "$(curl https://raw.githubusercontent.com/fuwwy/Discord-Screenshare-Linux/main/scripts/install.sh -sSfL)"
```
After installing restart discord, if it was running

**Note: the screen preview inside the screenshare popup will still be black, just click on one of them to trigger screensharing anyway.**

## Dependencies
```
pipewire pipewire-pulse discord xdg-desktop-portal xdg-desktop-portal-{insert desktop environment here} 
(ex. kde needs xdg-desktop-portal-kde, gnome needs xdg-desktop-portal-gnome)
```
