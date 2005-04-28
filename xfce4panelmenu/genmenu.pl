#!/usr/bin/perl

use strict;
use warnings;

use Data::Dumper;

my @paths = ("/usr/share/applications",
	     "/usr/local/share/applications",
	     "/usr/X11R6/share/applications",
	     "/usr/X11R6/share/gnome/applications",
	     "/usr/X11R6/share/gnome/apps/Graphics",);

my $menu = {
	Development => {
		type => "menu",
		entries => {

		},
	},
	Office => {
		type => "menu",
		entries => {

		},
	},
	Graphics => {
		type => "menu",
		icon => "xfce-graphics",
		entries => {

		},
	},
	Settings => {
		type => "menu",
		icon => "xfce-system-settings",
		entries => {

		},
	},
	Multimedia => {
		type => "menu",
 		icon => "xfce-multimedia",
		entries => {

		},
	},
	Games => {
		type => "menu",
		entries => {

		},
	},
	Education => {
		type => "menu",
		entries => {

		},
	},
	Network => {
		type => "menu",
 		icon => "xfce-internet",
		entries => {

		},
	},
	System => {
		type => "menu",
		entries => {

		},
	},
	Utilities => {
		type => "menu",
		entries => {

		},
	},

};

my $categories = {
	Development => $menu->{Development}->{entries},
	Settings => $menu->{Settings}->{entries},
	Graphics => $menu->{Graphics}->{entries},
	Network => $menu->{Network}->{entries},
	AudioVideo => $menu->{Multimedia}->{entries},
	Game => $menu->{Games}->{entries},
	Office => $menu->{Office}->{entries},
	System => $menu->{System}->{entries},
	Utility => $menu->{Utilities}->{entries},
	Calendar => $menu->{Office}->{entries},
	Science => $menu->{Education}->{entries},
};

sub insert_into_menu {
	my $desktop_path = shift;

	my %desktop_entry;

	open ENTRY, "<$desktop_path" or warn "$desktop_path $!\n";
	while (my $line = <ENTRY>) {
		chomp $line;
		$desktop_entry{name} = $1    if $line =~ m/^Name=(.*)/;
		$desktop_entry{gname_l} = $1 if $ENV{LANG} and $line =~ m/^GenericName\[$ENV{LANG}\]=(.*)/;
		$desktop_entry{gname} = $1   if $line =~ m/^GenericName=(.*)/;
		$desktop_entry{name_l} = $1  if $ENV{LANG} and $line =~ m/^Name\[$ENV{LANG}\]=(.*)/;
		$desktop_entry{comm} = $1    if $line =~ m/Comment=(.*)/;
		$desktop_entry{only} = $1    if $line =~ m/OnlyShowIn=(.*);/;
		$desktop_entry{comm_l} = $1  if $ENV{LANG} and $line =~ m/Comment\[$ENV{LANG}\]=(.*)/;
		$desktop_entry{cmd}  = $1    if $line =~ m/Exec=(.*)/;
		$desktop_entry{icon} = $1    if $line =~ m/Icon=(.*)/;
		$desktop_entry{term} = "no"  if $line =~ m/Terminal=flase/;
		$desktop_entry{icon} = "yes" if $line =~ m/Terminal=true/;
		if ($line =~ m/Categories=(.*)/) {
			my @cats = split ";", $1 ;
			$desktop_entry{cats} = \@cats;
		}
	}
	close ENTRY;

	foreach my $cat (@{$desktop_entry{cats}}) {
		if (defined $categories->{$cat}) {
			$categories->{$cat}->{$desktop_entry{cmd}} = {
				type => "app",
				name => $desktop_entry{name_l} ? $desktop_entry{name_l} : $desktop_entry{name},
				gname => $desktop_entry{gname_l} ? $desktop_entry{gname_l} : $desktop_entry{gname},
				comm => $desktop_entry{comm_l} ? $desktop_entry{comm_l} : $desktop_entry{comm},
				cmd => $desktop_entry{cmd},
				only => $desktop_entry{only},
				icon => $desktop_entry{icon},
				term => $desktop_entry{term},
			};
			last;
		}
	}
}

sub print_level {
	my $level = shift;

	my $level_l = "";

	foreach my $entry (sort keys %$level) {
		if ($level->{$entry}->{type} eq "menu") {
			if (!$level->{$entry}->{valid}) {
				next;
			}
			$level_l .= "<menu name=\"$entry\"";
			if ($level->{$entry}->{icon}) {
				$level_l .= " icon=\"$level->{$entry}->{icon}\"";
			}
			$level_l .= ">\n";

			$level_l .= print_level ($level->{$entry}->{entries});

			$level_l .= "</menu>\n";
		}
		if ($level->{$entry}->{type} eq "app") {
			if ($level->{$entry}->{only} && $level->{$entry}->{only} ne "XFCE") {
				next;
			}
			$level_l .= "<app ";
			if ($level->{$entry}->{name} && $level->{$entry}->{only}) {
				$level_l .= "name=\"$level->{$entry}->{gname}\"";
			} else {
				$level_l .= "name=\"$level->{$entry}->{name}\"";
			}
			$level_l .= " cmd=\"$level->{$entry}->{cmd}\"";
			if ($level->{$entry}->{icon}) {
				$level_l .= " icon=\"$level->{$entry}->{icon}\"";
			}
			if ($level->{$entry}->{term}) {
				$level_l .= " term=\"$level->{$entry}->{term}\"";
			}
			$level_l .= " />\n";
		}
	}
	return $level_l;
}

sub validate_level {
	my $level = shift;

	my $valid = 0;

	foreach my $entry (keys %$level) {
		if ($level->{$entry}->{type} eq "app") {
			$valid = 1;
		}
		if ($level->{$entry}->{type} eq "menu") {
			if (validate_level ($level->{$entry}->{entries})) {
				$level->{$entry}->{valid} = 1;
				$valid = 1;
			} else {
				$level->{$entry}->{valid} = 0;
			}
		}
	}

	return $valid;
}

foreach my $path (@paths) {
	chdir $path or next;

	opendir (DIR, ".");
	while (my $entry = readdir (DIR)) {
		insert_into_menu ($entry) if $entry =~ m/.*\.desktop$/;
	}
	closedir DIR;
}

validate_level ($menu);

my $level;

open MENU, "<$ARGV[0]" or die $!;

my $menu_xml = join "", <MENU>;

close MENU;

$level .= print_level ($menu);

$menu_xml =~ s/<!--.*?-->//sg;
$menu_xml =~ s/<include[^>]*?type=\"system\".*?\/>/$level/sg;

print $menu_xml;

