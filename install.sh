./plugins/*/*.sh

./build.sh && {
	cp -f rose /usr/bin
	mkdir -p /usr/share/themes/rose
	cp style.css /usr/share/themes/rose/
	cp rose-mklink /usr/bin
}
