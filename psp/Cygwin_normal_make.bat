@echo off

echo Setting ENV...
set user=user
set home=C:/cygwin/home/user
set PSPDEV=/usr/local/pspdev
set PSPSDK=/usr/local/pspdev/psp/sdk
set path=%PSPDEV%;%PSPDEV%/bin;C:/cygwin/bin;C:/cygwin/user/bin;C:/cygwin/usr/local/bin;%path%

chdir "C:\cygwin\bin"

umount -s --remove-all-mounts
mount -s -b -f "C:/cygwin/" /
mount -s -b -f "C:/cygwin/bin" /bin
mount -s -b -f "C:/cygwin/etc" /etc
mount -s -b -f "C:/cygwin/lib" /lib
mount -s -b -f "C:/cygwin/usr" /usr
mount -s -b -f "C:/cygwin/var" /var
mount -s -b -f "C:/cygwin/home" /home
mount -s -b -f "C:/cygwin/bin" /usr/bin
mount -s -b -f "C:/cygwin/lib" /usr/lib
mount -c -s /cygdrive
mount -c -u /cygdrive

chdir "C:\cygwin\home\user"

echo Launching bash...


bash --login -i -c "cd \"proquake_dev\WinQuake\psp\"; make -f MakefileNormal"
cmd
