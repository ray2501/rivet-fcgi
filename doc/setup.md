Setup
=====

My test environment is openSUSE. The document is a note for myself.

It looks like:

    Web server <---> Spawn-fcgi <---> rivet-fcgi


# Spawn-fcgi

I use systemd to manage spawn-fcgi service.
Add a file `spawnfcgi.service` in the `/usr/lib/systemd/system` folder.

For unix socket -

    [Unit]
    Description=Spawn FCGI service
    After=nss-user-lookup.target

    [Service]
    Type=forking
    Environment=WORKERS=3
    ExecStart=/usr/bin/spawn-fcgi \
        -F ${WORKERS} \
        -u wwwrun \
        -g www \
        -s /var/run/%p.sock \
        -P /var/run/%p.pid \
        -- /usr/bin/rivet-fcgi
    Restart=on-failure
    RestartSec=5

    [Install]
    WantedBy=multi-user.target

For Tcp socket:

    [Unit]
    Description=Spawn FCGI service
    After=nss-user-lookup.target

    [Service]
    Type=forking
    Environment=WORKERS=3
    ExecStart=/usr/bin/spawn-fcgi \
        -F ${WORKERS} \
        -u wwwrun \
        -g www \
        -a 127.0.0.1 -p 9000 \
        -P /var/run/%p.pid \
        -- /usr/bin/rivet-fcgi
    Restart=on-failure
    RestartSec=5

    [Install]
    WantedBy=multi-user.target

In the config, -u to specify user and -g to specify group.
If users want to use Nginx,
then change user to `nginx` and group to `nginx`.
If users want to use Lighttpd,
then change user to `lighttpd` and group to `lighttpd`.

You can use `cgi-fcgi` (in openSUSE FastCGI package) to test.
`cgi‐fcgi` is a CGI/1.1 program that communicates with an already‐running
FastCGI application in order to respond to an HTTP request.

For example, if your script is hello.rvt and in /srv/www/htdocs, then

    SCRIPT_NAME=/hello.rvt \
    SCRIPT_FILENAME=/srv/www/htdocs/hello.rvt \
    REQUEST_METHOD=GET \
    cgi-fcgi -bind -connect 127.0.0.1:9000

So you can use `cgi‐fcgi` to do simple test by using command line interface.


# Web server

## Apache Http Server

Apache Http Server requries `mod_fcgid` to support FastCGI Protocol,
so you need to install it.

    sudo zypper in apache2-mod_fcgid

Then enable below modules:

    sudo a2enmod proxy
    sudo a2enmod proxy_fcgi
    sudo a2enmod setenvif
    sudo a2enmod fcgid

Edit `mod_fcgid.conf` file in the `/etc/apache2/conf.d` folder.

For unix socket -

    DirectoryIndex index.rvt index.tcl
    <FilesMatch "\.(rvt|tcl)$">
        SetHandler "proxy:unix:/var/run/spawnfcgi.sock|fcgi://localhost/"
        #CGIPassAuth on
    </FilesMatch>

For Tcp socket -

    DirectoryIndex index.rvt index.tcl
    <FilesMatch "\.(rvt|tcl)$">
        SetHandler "proxy:fcgi://127.0.0.1:9000/"
        #CGIPassAuth on
    </FilesMatch>

Then restart service and test.


## Nginx

Add a file `spawnfcgi.conf` in the `/etc/nginx` folder.

For unix socket -

    location ~ \.(rvt|tcl)$ {
        gzip off;
        root /srv/www/htdocs;
        try_files $uri/ $uri =404;
        fastcgi_pass unix:/var/run/spawnfcgi.sock;
        include /etc/nginx/fastcgi_params;
        fastcgi_param SCRIPT_FILENAME  $document_root$fastcgi_script_name;
    }

For Tcp socket -

    location ~ \.(rvt|tcl)$ {
        gzip off;
        root /srv/www/htdocs;
        try_files $uri/ $uri =404;
        fastcgi_pass 127.0.0.1:9000;
        include /etc/nginx/fastcgi_params;
        fastcgi_param SCRIPT_FILENAME  $document_root$fastcgi_script_name;
    }

Then add below config to your `server` section in `nginx.conf` or related files.

    include spawnfcgi.conf;

Then restart service and test.


## Lighttpd

You should enable `mod_fastcgi` module.

You can config in `/etc/lighttpd/modules.conf` or `/etc/lighttpd/lighttpd.conf`
to add `mod_fastcgi` to  `server.modules`.

Then modify `/etc/lighttpd/lighttpd.conf`.

For unix socket -

    fastcgi.server = (
    ".rvt" =>
    (( "socket" => "/var/run/spawnfcgi.sock",
        "docroot" => "/srv/www/htdocs"
    )),
    ".tcl" =>
    (( "socket" => "/var/run/spawnfcgi.sock",
        "docroot" => "/srv/www/htdocs"
    ))
    )

For Tcp socket -

    fastcgi.server = (
    ".rvt" =>
    (( "host" => "127.0.0.1",
        "port" => 9000,
        "docroot" => "/srv/www/htdocs"
    )),
    ".tcl" =>
    (( "host" => "127.0.0.1",
        "port" => 9000,
        "docroot" => "/srv/www/htdocs"
    ))
    )

Then restart service and test.


## CGI

If CGI is supported by your web server (and FastCGI is not), it is still
possible to run rivet-fcgi or FastCGI applications. The method is to use
`cgi-fcgi` to be a bridge.

Please notice, it is necessary that web server has to pass environment
variable `SCRIPT_FILENAME` to CGI program.

Write a script `fastcgi` in `/usr/local/bin` folder.
This script must have the execute permission.

Below is an example:

	#!/usr/bin/cgi-fcgi -f
	-bind -connect 127.0.0.1:9000

Then setup web server, let this script `fastcgi` to be an interpreter
for your CGI program.

This method maybe has performance impact, but now you can run FastCGI
applications in your web server.


