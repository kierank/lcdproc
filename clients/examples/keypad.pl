#!/usr/bin/perl -w

# keypad.pl - an keypad test client for LCDproc

#
# This client for LCDproc displays the tail of a specified file.
# It's possible to change the visible part of the file with LCDproc
# controlled keys
#
#
# Copyright (c) 2002, David Glaude
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
#

use IO::Socket;
use Fcntl;

############################################################
# Configurable part. Set it according your setup.
############################################################

# Host which runs lcdproc daemon (LCDd)
$HOST = "localhost";

# Port on which LCDd listens to requests
$PORT = "13666";

############################################################
# End of user configurable parts
############################################################

print "This LCDproc client let you test the input of your driver.\n";
print "It is very simplistic and just request every key from A to Y.\n";
print "\n";
print "It is suggested to add the following to your LCDd.conf:\n";
print "\n";
print " [Input] \n";
print " FreePauseKey=yes \n";
print " FreeBackKey=yes \n";
print " FreeForwardKey=yes \n";
print " FreeMainMenuKey=yes \n";
print "\n";



# Connect to the server...
$remote = IO::Socket::INET->new(
		Proto     => "tcp",
		PeerAddr  => $HOST,
		PeerPort  => $PORT,
	)
	|| die "Cannot connect to LCDproc port\n";

# Make sure our messages get there right away
$remote->autoflush(1);

sleep 1;	# Give server plenty of time to notice us...

print $remote "hello\n";
# Note: it's good practice to listen for a response after a print to the
# server even if there isn't meant to be one. If you don't, you may find
# your program crashes after running for a while when the buffers fill up.
my $lcdresponse = <$remote>;
print $lcdresponse;


# Turn off blocking mode...
fcntl($remote, F_SETFL, O_NONBLOCK);


# Set up some screen widgets...
print $remote "client_set name {Keypad}\n";
$lcdresponse = <$remote>;
print $remote "screen_add tail\n";
$lcdresponse = <$remote>;
print $remote "screen_set tail name {Keypad}\n";
$lcdresponse = <$remote>;
print $remote "widget_add tail title title\n";
$lcdresponse = <$remote>;
print $remote "widget_set tail title {Keypad}\n";
$lcdresponse = <$remote>;
print $remote "widget_add tail 1 string\n";
$lcdresponse = <$remote>;

# NOTE: You have to ask LCDd to send you keys you want to handle
# We ask for key from A to Y like the output of MO 5*5 matrix keypad.
print $remote "client_add_key A\n";
$lcdresponse = <$remote>;
print $remote "client_add_key B\n";
$lcdresponse = <$remote>;
print $remote "client_add_key C\n";
$lcdresponse = <$remote>;
print $remote "client_add_key D\n";
$lcdresponse = <$remote>;
print $remote "client_add_key E\n";
$lcdresponse = <$remote>;
print $remote "client_add_key F\n";
$lcdresponse = <$remote>;
print $remote "client_add_key G\n";
$lcdresponse = <$remote>;
print $remote "client_add_key H\n";
$lcdresponse = <$remote>;
print $remote "client_add_key I\n";
$lcdresponse = <$remote>;
print $remote "client_add_key J\n";
$lcdresponse = <$remote>;
print $remote "client_add_key K\n";
$lcdresponse = <$remote>;
print $remote "client_add_key L\n";
$lcdresponse = <$remote>;
print $remote "client_add_key M\n";
$lcdresponse = <$remote>;
print $remote "client_add_key N\n";
$lcdresponse = <$remote>;
print $remote "client_add_key O\n";
$lcdresponse = <$remote>;
print $remote "client_add_key P\n";
$lcdresponse = <$remote>;
print $remote "client_add_key Q\n";
$lcdresponse = <$remote>;
print $remote "client_add_key R\n";
$lcdresponse = <$remote>;
print $remote "client_add_key S\n";
$lcdresponse = <$remote>;
print $remote "client_add_key T\n";
$lcdresponse = <$remote>;
print $remote "client_add_key U\n";
$lcdresponse = <$remote>;
print $remote "client_add_key V\n";
$lcdresponse = <$remote>;
print $remote "client_add_key W\n";
$lcdresponse = <$remote>;
print $remote "client_add_key X\n";
$lcdresponse = <$remote>;
print $remote "client_add_key Y\n";
$lcdresponse = <$remote>;

# Forever, we should do stuff...
while(1)
{
	# Handle input...  (spew it to the console)
        # Also, certain keys scroll the display
	while(defined($input = <$remote>)) {
	    if ( $input =~ /^success$/ ) { next; }
	    print $input;

	    # Make sure we handle each line...
	    @lines = split(/\n/, $input);
	    foreach $line (@lines)
	    {
		if($line =~ /^key (\S)/)  # Keypresses are useful.
		{
		    $key = $1;
		    print $remote "widget_set tail 1 1 2 {Key is $key}\n";
		    my $lcdresponse = <$remote>;
		}
	    }
	}
}

close ($remote)            || die "close: $!";
exit;

