# Rosenrot

Rosenrot is a small browser forked from an earlier version of [rose](https://github.com/mini-rose/rose). It has some additional quality of life improvements tailored to my (@NunoSempere) tastes and setup, and detailed installation instructions for Debian 12.

![](https://raw.githubusercontent.com/NunoSempere/rosenrot-browser/master/images/7-hello-world-search.png)

![](https://raw.githubusercontent.com/NunoSempere/rosenrot-browser/master/images/6-hello-world.png)

### Installation and usage

You can see detailed instructions [here](./user-scripts/debian-12/install-with-dependencies.sh), for Debian 12 in particular—though they should generalize easily to other distributions. 

The general steps are to install dependencies, and then

```
make build
make install # or sudo make install
rose
```

You can also collect some profiling info, and then use that to get a perhaps faster version:

```
make fast ## will ask you to use the browser for a bit
make install
rose
```

You can also create a rose.desktop file so that it will show up in your desktop environment. You can see this documented [here](./user-scripts/debian-12/install-with-dependencies.sh).

## Features

- Tabs, cookies, caching
- Minimal ui, autohiding elements
- ~454L core code (the rose.c file)
- Customize appearance of the browser through css
- Built-in rose-mklink script for in-shell static links
- Optional adblocking through [wyebadblock](https://github.com/jun7/wyebadblock)
- Plugin system, seeded with:
  - Libre redirect: Redirect annoying websites to open source frontends
  - Readability: Strip webpages of unnecessary elements for ease of reading with a custom shortcut
  - Custom style: Override the css of predetermined websites
  - Max number of tabs (by default 8), configurable.
  - Stand in plugin: Mimick function definitions which do nothing for the above plugins so that they can be quickly removed

You can see some screenshots in the [images](./images) folder.

## Comparisons 

### Relationship with [rose](https://github.com/mini-rose/rose)

- Rose is a small browser based on webkit2gtk. Previously, it described itself as aiming to be a "basement for creating your own browser using [the] gtk and webkit libraries". It has since diverged into a more featureful small browser with lua bindings, and rebased its history. You can see the original, minimal version [here](https://github.com/NunoSempere/rosenrot-browser/blob/a45d1c70f58586fed97df70650e5d066b73d0a0d/rose.c).
- The current version offers compilation with both GTK3 and GTK4, and an up to date version of webkit.
- Rosenrot is my (@NunoSempere's) fork from that earlier minimal rose. It has accumulated quality of life features and, honestly, cruft, that I like, like a "readability" plugin that simplifies annoying websites like [Matt Levine's Money Stuff newsletter](https://www.bloomberg.com/opinion/articles/2022-10-18/matt-levine-s-money-stuff-credit-suisse-was-a-reverse-meme-stock). It also incorporates ad-blocking.
- Rosenrot is also a song by the German hardcore rock band [Rammstein](https://www.youtube.com/watch?v=af59U2BRRAU).

### Comparison with [surf](https://git.suckless.org/surf/file/surf.c.html)

- Surf is another browser based on GTK/Webkit, from the suckless community. 
- It is significantly more complex: surf.c has [2170](https://git.suckless.org/surf/file/surf.c.html) lines, vs rose.c's [454](https://git.nunosempere.com/open.source/rosenrot/src/branch/master/rose.c).
- I find its code messier and harder to understand.
- Conversely, surf has significantly more configuration options, and digs deeper into webkit internals.
- Anecdotically, surf feels slower, though I haven't tested this rigorously.
- surf has a larger community, with patches and modifications.
- surf is more opinionated, but also less amateurish.
- Like rosenrot until very recently, it [uses](https://git.suckless.org/surf/file/config.mk.html#l15) an obsolete & deprecated version of [webkit](https://blogs.gnome.org/mcatanzaro/2023/03/21/webkitgtk-api-for-gtk-4-is-now-stable/)
- My recommendation would be to use rosenrot, and if you find some feature missing, either look how surf does it and import it to rose, or move to surf.
  - But then again, I've built rosenrot to cater to my own tastes, so I'd say that.

## Folk wisdom

Of general interest:

- I just found out that you can inspect a GTK application with the GTK explorer if you set a certain command-line variable. Try this with `make inspect`.
- Static variables keep their value between invocations.
- By default the searchbar is pretty gigantic. I've made this so because I'm a bit myopic, but also work with my laptop in a laptop stand. Anyways, if you are a more normal person you can change this in the style.css.
- The style.css usage isn't updated until installation. This is because by default rose uses the theme located in /usr/share/themes/rose/style.css, and that file isn't updated until make install.

The "architecture" of the application looks as follows:

![](https://raw.githubusercontent.com/NunoSempere/rosenrot-browser/master/images/0-architecture.png)

Specific to my own system:

## webkit2gtk-4.0 vs webkit2gtk-4.1 vs webkit2gtk-6.0

See [this blog post](https://blogs.gnome.org/mcatanzaro/2023/03/21/webkitgtk-api-for-gtk-4-is-now-stable/) for details. webkit2gtk-4.0  is deprecated, webkit2gtk-4.1 is the current [stable](https://webkitgtk.org/reference/webkit2gtk/stable/index.html) release and uses GTK3. webkit2gtk-6.0 is the current [unstable](https://webkitgtk.org/reference/webkitgtk/unstable/index.html) release, and uses GTK4.

Migration instructions for migration to webkit2gtk-6 and GTK4 can be seen [here](https://github.com/WebKit/WebKit/blob/ed1422596dce5ff012e64a38faf402ac1674fc7e/Source/WebKit/gtk/migrating-to-webkitgtk-6.0.md) and [here](https://docs.gtk.org/gtk4/migrating-3to4.html).

Rosenrot is currently on the stable webkit2gtk-4.1 release using GTK3. It has plans to eventually migrate to webkit2gtk-6.0 eventually but not soon.

## Ubuntu 20.04

A previous version of this repository was based on Ubuntu 20.04. You can still see documentation for that distribution [here](https://git.nunosempere.com/open.source/rosenrot/src/commit/8a1e0be30df52d5a21109297fd5bbc20efec1b3b), particularly a video installing rosenrot in a fresh Ubuntu 20.04 virtual machine [here](https://video.nunosempere.com/w/t3oAvJLPHTSAMViQ6zbwTV). However, that uses the webkit2gtk-4.0 library. Instead, I recommend adapting the Debian 12 instructions.

