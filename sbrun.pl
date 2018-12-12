#!/usr/bin/perl
# ------------------------------------------------------------------------
# Copyright Stephen Stebbing 2014.
#
# This script runs the sbread utility to read the power data from a sunnyboy 
# solar inverter and then uses the curl utility to submit the data to a webserver.
#
# Please ensure that the address and serial number for the inverter are set
# in the config section below. 
# See http://www.on4akh.be/SMA-read.html for details of how to convert the
# serial number to the required hex string.
#
# Note that sometimes the sbread script fails (suspected due to low bluetooth
# signal strength). This script attempts to run sbread $retries number of times
# before failing.
#
# On success, 0 is returned. On failure a message is written and return
# value is non-zero
# ------------------------------------------------------------------------
use strict;
use warnings;
# ------------------------------------------------------------------------
# config - change to suit installation
# sunnyboy address and serial as required by the sbread utility.
my $sbAddr='00:80:25:A6:77:60';
my $sbSerial='7E:F9:04:9F';
# url for sending power reports to
my $powerURL="http://ss.com/solar/report.php";
# ------------------------------------------------------------------------
# config - generally doesn't need to change
# path to the curl utility
my $curlUtil='/usr/bin/curl';
# path to the sbread utility
my $sbreadUtil='/usr/local/bin/sbread';
# number of retries to get a valid response from sbread
my $retries=6;
# number of seconds to delay between retries
my $retryDelaySec=5;
# ------------------------------------------------------------------------
my $sbreadCmd="$sbreadUtil -address $sbAddr -serial $sbSerial -b";
# curl command for sending power report, this is run thru sprintf to set the power value
my $curlCmd="$curlUtil --data \"powernow=%d\" $powerURL";


sub sendPowerData($)
{
  my $cc=sprintf($curlCmd,shift);
  system($cc);
  if($? != 0){
    die "Curl failed: $cc";
  }
}


# try to run sbread up to $retries times
for(my $i=0; $i<$retries; ++$i){
#  print "trying $i...\n";
  my $reading=`$sbreadCmd`;
  if($?==0){
    # success
    # reading is eg '1234,3.4' being power, energy-today
    $reading =~ s/,.*$//;
    sendPowerData($reading);
    exit 0;
  }
  sleep($retryDelaySec);
}

die "Retries exceeded";




