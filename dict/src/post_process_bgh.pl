#!/usr/bin/env perl

use strict;

binmode(STDOUT);

my @list;
while (<>) {
    chomp;
    push(@list, $_);
}

my $prekey;
my @values;
for my $line (sort @list) {
    if ($line =~ /^(\S+)\s+(\S+)$/) {
	my ($key, $value) = ($1, $2);
	if ($key ne $prekey) {
	    print $prekey, ' ', join('', @values), "\n" if $prekey;
	    $prekey = $key;
	    @values = ();
	}
	push(@values, $value);
    }
}

print $prekey, ' ', join('', @values), "\n" if $prekey;
