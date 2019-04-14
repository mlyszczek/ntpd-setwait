[kursg-meta]: # (order: 1)

Synopsis
========

**ntpd-setwait** is very small program that ensures time is synchronized before
**ntpd** daemon can be started. This is *not* replacement for **ntpd**, you
still need **ntpd** installed, it's just **ntpd** will not be started until
**ntpd-setwait** sets system time from ntp server.

Program is designed for embedded systems which might not have RTC and back
up battery, which results in time reset each time board reboots or power is
cut off and internet may or may not be available at time of boot up. Desktop
and server users rather won't find it usefull.

Why
===

Well, **openntpd** has *-s* argument, which will cause system time to be set
immediately when **openntpd** starts. Problem is, that **openntpd** after 15
seconds (in case no internet) will forfeit request to set system time, and
will fork into background leaving you stuck in 1970 making little forward
jumps with **adjtime**(), which will take forever to get to present date.

This is where **ntpd-setwait** comes in. It will hang in infinite loop, waiting
for internet, when that is available, it reads current time from ntp server,
sets current system time. If all steps succeds **ntpd-setwait** starts **ntpd**
of your choosing to keep the time synchronized from now on.

Description
===========

Description and usage can be found in [manual page][1].

Dependencies
============

None, just **unix** system and any **ntpd** daemon.

Compile and install
===================

Program uses autotools, so instalation is very easy:

~~~{.sh}
$ ./autogen.sh
$ ./configure
$ make
# make install
~~~

**autogen.sh** can be ommited if you have downloaded tarball. That script
must be called only if you cloned sources from **git** repository.

License
=======

Program is licensed under BSD 2-clause license. See LICENSE file for details.

Contact
=======

Michał Łyszczek [michal.lyszczek@bofc.pl][2]

See also
========

* [git repository][3] to browse sources online

[1]: https://ntpd-setwait.kurwinet.pl/manuals/ntpd-setwait.1.html
[2]: mailto:michal.lyszczek@bofc.pl
[3]: https://git.kurwinet.pl/ntpd-setwait
