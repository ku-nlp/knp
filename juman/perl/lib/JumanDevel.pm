package JumanDevel;

# 2012/5/28 kuro@i.kyoto-u.ac.jp

use Exporter;
use utf8;

@ISA = qw(Exporter);

@EXPORT = qw(read_juman_entry write_juman_entry write_LD_entry);

######################################################################
sub read_juman_entry
{
    # JUMANのS式の1エントリを読む

    my($input) = @_;
    my($top_midashi, $midashi, $yomi, $pos, $pos2, $conj, $sem, $comment);
    my($open_p, $close_p);
    my(@m);

    chomp($input);
    
    $open_p = @{$input =~ [/\(/g]};
    $close_p = @{$input =~ [/\)/g]};
    return ("ERROR") if ($open_p != $close_p);

    if ($input =~ s/\s*\;\s*([^\)]+)$//) {
	$comment = $1;
    } else {
	$comment = "";
    }

    # "(あいず 1.6)" -> "あいず:1.6"
    $input =~ s/\(([^ \(\)]+) ([0-9\.]+)\)/\1:\2 /g;
    $input =~ s/  / /g;
    $input =~ s/ \)/\)/;

    if ($input =~ /^\(([^ \(\)]+) \(([^ \(\)]+)/) {
	$pos = $1; $pos2 = $2;
    } elsif ($input =~ /^\(([^ \(\)]+)/) {
	$pos = $1; $pos2 = "";
    }
	
    $input =~ /見出し語 ([^\)]+)/;
    $midashi = $1;
    @m = split(/ /, $midashi);
    $top_midashi = shift(@m);
    $top_midashi =~ s/\:[0-9\.]+$//;

    $input =~ /読み ([^\)]+)/;
    $yomi = $1;

    if ($input =~ /活用型 ([^ \)]+)/) {
	$conj = $1;
    } else {
	$conj = "";
    }

    if ($input =~ /意味情報 \"([^\"]+)\"/) {
	$sem = $1;
    } else {
	$sem = "";
    }

    return ($top_midashi, $midashi, $yomi, $pos, $pos2, $conj, $sem, $comment);
}

######################################################################
sub write_juman_entry
{
    # JUMANの1エントリをS式で出力

    my($midashi, $yomi, $pos, $pos2, $conj, $sem) = @_;
    my($string);

    $midashi =~ s/([^ \(\)]+):1\.0 /\1 /g;
    $midashi =~ s/([^ \(\)]+):1 /\1 /g;
    $midashi =~ s/([^ \(\)]+):1\.0$/\1/g;
    $midashi =~ s/([^ \(\)]+):1$/\1/g;
    $midashi =~ s/([^ \(\)]+):([0123456789\.]+)/\(\1 \2\)/g;
    $string = "(読み $yomi)(見出し語 $midashi)";
    $string .= "(活用型 $conj)" if ($conj);
    $string .= "(意味情報 \"$sem\")" if ($sem);
    $string = "($string)";
    $string = "($pos2 $string)" if ($pos2);
    $string = "($pos $string)";
    return $string;
}

######################################################################
sub write_LD_entry
{
    my($type, $rep, $std, $yomi, $midashi, $pos, $pos2, $conj, $sem, $comment) = @_;
    my(@etc);

    $comment =~ s/\</&lt;/g;
    $comment =~ s/\>/&gt;/g;
    
    # LD用出力
    printf "<LDEntry type=\"$type\">\n";
    printf "  <RepForm>$rep</RepForm>\n";
    printf "  <StdForm>$std</StdForm>\n" if ($std);
    printf "  <Yomi>$yomi</Yomi>\n";
    printf "  <Form>$midashi</Form>\n";
    printf "  <POS>$pos"; printf ":$pos2" if ($pos2); printf "</POS>\n";
    printf "  <Conj>$conj</Conj>\n" if ($conj); 
    @etc = ();
    foreach $i (split(/ /, $sem)) {
	if ($i =~ /:/ ) {
	    $i =~ /([^:]+):(.+)/; $type = $1; $info = $2;
	    printf "  <Sem type=\"$type\">$info</Sem>\n";
	} else {
	    push(@etc, $i);
	}
    }
    printf "  <Sem type=\"etc\">%s</Sem>\n", join(" ", @etc) if (@etc);
    printf "  <Comment>$comment</Comment>\n" if ($comment); 
    printf "</LDEntry>\n";
}

######################################################################

1;

