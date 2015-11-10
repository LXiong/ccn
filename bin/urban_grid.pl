#!/usr/bin/perl


if ($#ARGV != 7)
{
    print "usage: urban_grid.pl num_rows num_cols grid_width street_width park_row park_col min_building_height max_building_height";
    exit;
}

$num_rows = $ARGV[0];
$num_cols = $ARGV[1];
$grid_width = $ARGV[2];
$street_width = $ARGV[3];
$park_row = $ARGV[4];
$park_col = $ARGV[5];
$min_building_height = $ARGV[6];
$max_building_height = $ARGV[7];

if ($park_row >= $num_rows)
{
    print "park row must be less than number of rows";
    exit;
}

if ($park_col >= $num_cols)
{
    print "park column must be less than number of columns";
    exit;
}

print "<?xml version=\"1.0\"?>\n";
print "<Site Name=\"GRID\" id=\"0\" ReferencePoint=\"0.0 0.0 0\" CoordinateType=\"cartesian\" part=\"1\" totalParts=\"1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"file:///c:qualnet/qualnet-road.xsd\">\n";
print "<Region id=\"1\">\n";
print "<position>0 0 0</position>\n";
print "<position>0 "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>"; print $num_cols*$grid_width; print " "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>"; print $num_cols*$grid_width; print " 0 0</position>\n";
print "</Region>\n";

print "$1";

for ($row = 0; $row <= $num_rows; $row++)
{
    for ($col = 0; $col <= $num_cols; $col++)
    {
        print "<Intersection id=\"$row.$col\">\n";
        print "<location>"; print $col*$grid_width; print " "; print $row*$grid_width; print " 0</location>\n";

        print "<objects id=\"O$row.$col\">\n";
        if ($col != 0)
        {
            print "<reference>$row."; print $col-1; print "-$row.$col</reference>\n";
        }
        if ($col != $num_cols)
        {
            print "<reference>$row.$col-$row."; print $col+1; print "</reference>\n";
        }
        if ($row != 0)
        {
            print "<reference>"; print $row-1; print ".$col-$row.$col</reference>\n";
        }
        if ($row != $num_rows)
        {
            print "<reference>$row.$col-"; print $row+1; print ".$col</reference>\n";
        }

        if (($row == $park_row and $col == $park_col)
            or ($row == $park_row and $col == $park_col+1)
            or ($row == $park_row+1 and $col == $park_col)
            or ($row == $park_row+1 and $col == $park_col+1))
        {
            print "<reference>Park</reference>\n";
        }

        # put stations at 4 corners
        if ($row == 0 and $col == 0)
        {
            print "<reference>Station1</reference>\n";
        }

        if ($row == $num_rows and $col == 0)
        {
            print "<reference>Station2</reference>\n";
        }

        if ($row == 0 and $col == $num_cols)
        {
            print "<reference>Station3</reference>\n";
        }

        if ($row == $num_rows and $col == $num_cols)
        {
            print "<reference>Station4</reference>\n";
        }

        print "</objects>\n";
        print "</Intersection>\n";

        if ($col != $num_cols)
        {
            print "<Street_Segment id=\"$row.$col-$row."; print $col+1; print"\">\n";
            print "<firstNode objectRef=\"$row.$col\">"; print $col*$grid_width; print " "; print $row*$grid_width; print " 0</firstNode>\n";
            print "<lastNode objectRef=\"$row."; print $col+1; print "\">"; print (($col+1)*$grid_width); print " "; print $row*$grid_width; print " 0</lastNode>\n";
            print "<width>$street_width</width>\n";
            print "</Street_Segment>\n";
        }

        if ($row != $num_rows)
        {
            print "<Street_Segment id=\"$row.$col-"; print $row+1; print ".$col\">\n";
            print "<firstNode objectRef=\"$row.$col\">"; print $col*$grid_width; print " "; print $row*$grid_width; print " 0</firstNode>\n";
            print "<lastNode objectRef=\""; print $row+1; print ".$col\">"; print $col*$grid_width; print " "; print (($row+1)*$grid_width); print " 0</lastNode>\n";
            print "<width>$street_width</width>\n";
            print "</Street_Segment>\n";
        }

        # add building unless adding park
        if ($row != $num_rows and $col != $num_cols and not ($row == $park_row and $col == $park_col))
        {
            $x_width = rand(0.5) + 0.25;
            $minx = ($col + (1-$x_width)*(0.25+rand(0.5))) * $grid_width;
            $maxx = $minx + ($grid_width * $x_width);
            $y_width = rand(0.5) + 0.25;
            $miny = ($row + (1-$y_width)*(0.25+rand(0.5))) * $grid_width;
            $maxy = $miny + ($grid_width * $y_width);
            $building_height = $min_building_height + rand($max_building_height - $min_building_height);

            print "<Building id=\"building$row.$col\">\n";
            print "<face id=\"$row.$col"; print "face1\">\n";
            print "<position>$minx $miny 0.0</position>\n";
            print "<position>$minx $maxy 0.0</position>\n";
            print "<position>$maxx $maxy 0.0</position>\n";
            print "<position>$maxx $miny 0.0</position>\n";
            print "</face>\n";
            print "<face id=\"$row.$col"; print "face2\">\n";
            print "<position>$minx $maxy 0.0</position>\n";
            print "<position>$minx $miny 0.0</position>\n";
            print "<position>$minx $miny $building_height</position>\n";
            print "<position>$minx $maxy $building_height</position>\n";
            print "</face>\n";
            print "<face id=\"$row.$col"; print "face3\">\n";
            print "<position>$maxx $miny 0.0</position>\n";
            print "<position>$maxx $maxy 0.0</position>\n";
            print "<position>$maxx $maxy $building_height</position>\n";
            print "<position>$maxx $miny $building_height</position>\n";
            print "</face>\n";
            print "<face id=\"$row.$col"; print "face4\">\n";
            print "<position>$minx $miny 0.0</position>\n";
            print "<position>$maxx $miny 0.0</position>\n";
            print "<position>$maxx $miny $building_height</position>\n";
            print "<position>$minx $miny $building_height</position>\n";
            print "</face>\n";
            print "<face id=\"$row.$col"; print "face5\">\n";
            print "<position>$maxx $maxy 0.0</position>\n";
            print "<position>$minx $maxy 0.0</position>\n";
            print "<position>$minx $maxy $building_height</position>\n";
            print "<position>$maxx $maxy $building_height</position>\n";
            print "</face>\n";
            print "<face id=\"$row.$col"; print "face6\">\n";
            print "<position>$minx $miny $building_height</position>\n";
            print "<position>$maxx $miny $building_height</position>\n";
            print "<position>$maxx $maxy $building_height</position>\n";
            print "<position>$minx $maxy $building_height</position>\n";
            print "</face>\n";
            print "</Building>\n";
        }
    }
}

#add park

print "<Park id=\"Park\">\n";
print "<position ExitIntersectionId=\"$park_row.$park_col\">"; print $park_col*$grid_width; print " "; print $park_row*$grid_width; print " 0</position>\n";
print "<position ExitIntersectionId=\"$park_row."; print $park_col+1; print "\">"; print (($park_col+1)*$grid_width); print " "; print $park_row*$grid_width; print " 0</position>\n";
print "<position ExitIntersectionId=\""; print $park_row+1; print "."; print $park_col+1; print "\">"; print (($park_col+1)*$grid_width); print " "; print (($park_row+1)*$grid_width); print " 0</position>\n";
print "<position ExitIntersectionId=\""; print $park_row+1; print ".$park_col\">"; print $park_col*$grid_width; print " "; print (($park_row+1)*$grid_width); print " 0</position>\n";
print "<representative>"; print $park_col*$grid_width; print " "; print $park_row*$grid_width; print " 0</representative>\n";
print "</Park>\n";


#add stations at 4 corners
print "<Station id=\"Station1\">\n";
print "<position ExitIntersectionId=\"0.0\">0 0 0</position>\n";
print "<position>12 0 0</position>\n";
print "<position>12 12 0</position>\n";
print "<position>0 12 0</position>\n";
print "<representative>0 0 0</representative>\n";
print "</Station>\n";

print "<Station id=\"Station2\">\n";
print "<position ExitIntersectionId=\"$num_rows.0\">0 "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>12 "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>12 "; print ($num_rows*$grid_width-12); print " 0</position>\n";
print "<position>0 "; print ($num_rows*$grid_width-12); print " 0</position>\n";
print "<representative>0 "; print $num_rows*$grid_width; print " 0</representative>\n";
print "</Station>\n";

print "<Station id=\"Station3\">\n";
print "<position ExitIntersectionId=\"0.$num_cols\">"; print $num_cols*$grid_width; print " 0 0</position>\n";
print "<position>"; print $num_cols*$grid_width; print " 12 0</position>\n";
print "<position>"; print ($num_cols*$grid_width-12); print " 12 0</position>\n";
print "<position>"; print ($num_cols*$grid_width-12); print " 0 0</position>\n";

print "<representative>"; print $num_cols*$grid_width; print " 0 0</representative>\n";
print "</Station>\n";

print "<Station id=\"Station4\">\n";
print "<position ExitIntersectionId=\"$num_rows.$num_cols\">"; print $num_cols*$grid_width; print " "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>"; print ($num_cols*$grid_width-12); print " "; print $num_rows*$grid_width; print " 0</position>\n";
print "<position>"; print ($num_cols*$grid_width-12); print " "; print ($num_rows*$grid_width-12); print " 0</position>\n";
print "<position>"; print $num_cols*$grid_width; print " "; print ($num_rows*$grid_width-12); print " 0</position>\n";
print "<representative>"; print $num_cols*$grid_width; print " "; print $num_rows*$grid_width; print " 0</representative>\n";
print "</Station>\n";

print "</Site>\n";
