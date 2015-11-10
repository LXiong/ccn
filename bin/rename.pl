#!/usr/bin/perl
# Define Variables

$HOME = $ENV{'QUALNET_HOME'};

$word_file = "$HOME/bin/substitutions.txt";

$filename = $ARGV[0];
if (!$filename) {
  print "Usage: rename filename\n";
  exit(1);
}

# load name change file
&get_substitutions;

# Open the new file and write information to it.
&load_file;
&update_file;
&write_file;

############################
# Get Data Number Subroutine

sub get_substitutions {
   open(WORD_FILE,"$word_file") || die $!;
   @substitutions = <WORD_FILE>;
   close(WORD_FILE);
   foreach $entry (@substitutions) {
       $entry =~ s/\n//g;
   }
#   foreach $entry (@substitutions) {
#       ($oldname, $newname) = split(/=/, $entry);
#       print "$oldname is replaced by $newname\n";
#   }
}

############################
# load file to be updated

sub load_file {
   open(CODE_FILE,"$filename") || die $!;
   @lines = <CODE_FILE>;
   close(CODE_FILE);
}

############################
# update file

sub update_file {
   $i=0;
#   foreach ($j = 0; $j <= @lines; $j++) {
#      if ($lines[$j] =~ /.*Use the latest version of the PARSEC compiler*/) {
#         print "removing $lines[$j]";
#         splice(@lines, $j, 2);
#         $j--;
#      }
#      elsif ($lines[$j] =~ /.*\$Id:.*/) {
#         print "also removing $lines[$j]";
#         splice(@lines, $j, 1);
#         $j--;
#      }
#   }

   foreach $line (@lines) {
      #print "old## $line";
      foreach $entry (@substitutions) {
           ($oldname, $newname) = split(/=/, $entry);
           if ($line =~ s/$oldname/$newname/g) {
               $i++;
               print "\treplacing $oldname with $newname\n";
           }
       }
       #print "new## $line";
   }
   print "$i Substitutions in $filename.\n";
}

############################
# write updated file

sub write_file {
   open(CODE_FILE,">$filename") || die $!;
   foreach $line (@lines) {
       print CODE_FILE "$line";
   }
   close(CODE_FILE);
}
