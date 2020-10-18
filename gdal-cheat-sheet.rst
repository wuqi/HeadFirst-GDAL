.. highlight:: rst
.. _CheatSheet:

############################
GDAL Cheat Sheet
############################
本文为 ``GitHub`` 上的项目，原项目地址： https://github.com/dwtkns/gdal-cheat-sheet

********************
Vector operations
********************

**Get vector information**

::

    ogrinfo -so input.shp layer-name

Or, for all layers

::

    ogrinfo -al -so input.shp

**Print vector extent**

::

    ogrinfo input.shp layer-name | grep Extent

**List vector drivers**

::

    ogr2ogr --formats

**Convert between vector formats**

::

    ogr2ogr -f "GeoJSON" output.json input.shp

**Clip vectors by bounding box**

::

    ogr2ogr -f "ESRI Shapefile" output.shp input.shp -clipsrc 
            <x_min> <y_min> <x_max> <y_max>

**Clip one vector by another**

::

    ogr2ogr -clipsrc clipping_polygon.shp output.shp input.shp

**Reproject vector:**

::

    ogr2ogr output.shp -t_srs "EPSG:4326" input.shp

**Merge features in a vector file by attribute ("dissolve")**

::

    ogr2ogr -f "ESRI Shapefile" dissolved.shp input.shp -dialect sqlite -sql 
      "select ST_union(Geometry),common_attribute from input GROUP BY common_attribute"

**Merge vector files:**

::

    ogr2ogr merged.shp input1.shp
    ogr2ogr -update -append merged.shp input2.shp -nln merged

**Extract from a vector file based on query**

To extract features with STATENAME 'New York','New Hampshire', etc. from
states.shp

::

    ogr2ogr -where 'STATENAME like "New%"' states_subset.shp states.shp

To extract type 'pond' from water.shp

::

    ogr2ogr -where "type = pond" ponds.shp water.shp

**Subset & filter all shapefiles in a directory**

Assumes that filename and name of layer of interest are the same...

::

    ls -1 *.shp | sed 's/.shp//g' | xargs -n1 -I % ogr2ogr %-subset.shp %.shp 
       -sql "SELECT field-one, field-two FROM '%' WHERE field-one='value-of-interest'"
    
********************
Raster operations
********************

**Get raster information**

::

    gdalinfo input.tif

**List raster drivers**

::

    gdal_translate --formats

**Convert between raster formats**

::

    gdal_translate -of "GTiff" input.grd output.tif

**Convert 16-bit bands (Int16 or UInt16) to Byte type**
(Useful for Landsat 8 imagery...)

::

    gdal_translate -of "GTiff" -co "COMPRESS=LZW" -scale 0 65535 0 255 
        -ot Byte input_uint16.tif output_byte.tif

You can change '0' and '65535' to your image's actual min/max values to
preserve more color variation or to apply the scaling to other band
types - find that number with:

::

    gdalinfo -mm input.tif | grep Min/Max

**Convert a directory of files to a different raster format**

::

    ls -1 *.img | sed 's/.img//g' | xargs -n1 -I % gdal_translate 
            -of "GTiff" %.img %.tif

**Reproject raster:**

::

    gdalwarp -t_srs "EPSG:102003" input.tif output.tif

Be sure to add *-r bilinear* if reprojecting elevation data to prevent
funky banding artifacts.

**Georeference an unprojected image with known bounding coordinates:**

::

    gdal_translate -of GTiff -a_ullr <top_left_lon> <top_left_lat> 
        <bottom_right_lon> <bottom_right_lat> -a_srs EPSG:4269 input.png output.tif

**Clip raster by bounding box**

::

    gdalwarp -te <x_min> <y_min> <x_max> <y_max> input.tif clipped_output.tif

**Clip raster to SHP / NoData for pixels beyond polygon boundary**

::

    gdalwarp -dstnodata <nodata_value> -cutline input_polygon.shp 
        input.tif clipped_output.tif

**Crop raster dimensions to vector bounding box**

::

    gdalwarp -cutline cropper.shp -crop_to_cutline input.tif cropped_output.tif

**Merge rasters**

::

    gdal_merge.py -o merged.tif input1.tif input2.tif

Alternatively,

::

    gdalwarp input1.tif input2.tif merged.tif

Or, to preserve nodata values:

::

    gdalwarp input1.tif input2.tif merged.tif -srcnodata <nodata_value> 
        -dstnodata <merged_nodata_value>

**Stack grayscale bands into a georeferenced RGB**

Where LC81690372014137LGN00 is a Landsat 8 ID and B4, B3 and B2
correspond to R,G,B bands respectively:

::

    gdal_merge.py -co "PHOTOMETRIC=RGB" -separate LC81690372014137LGN00_B{4,3,2}.tif 
        -o LC81690372014137LGN00_rgb.tif

**Fix an RGB TIF whose bands don't know they're RGB**

::

    gdal_merge.py -co "PHOTOMETRIC=RGB" input.tif -o output_rgb.tif

**Export a raster for Google Earth**

::

    gdal_translate -of KMLSUPEROVERLAY input.tif output.kmz -co FORMAT=JPEG

**Raster calculation (map algebra)**

Average two rasters:

::

    gdal_calc.py -A input1.tif -B input2.tif --outfile=output.tif --calc="(A+B)/2"

Add two rasters:

::

    gdal_calc.py -A input1.tif -B input2.tif --outfile=output.tif --calc="A+B"

etc.

**Create a hillshade from a DEM**

::

    gdaldem hillshade -of PNG input.tif hillshade.png

Change light direction:

::

    gdaldem hillshade -of PNG -az 135 input.tif hillshade_az135.png 

Use correct vertical scaling in meters if input is projected in degrees

::

    gdaldem hillshade -s 111120 -of PNG input_WGS1984.tif hillshade.png

**Apply color ramp to a DEM**
First, create a color-ramp.txt file:
*(Height, Red, Green, Blue)*

::

        0 110 220 110
        900 240 250 160
        1300 230 220 170
        1900 220 220 220
        2500 250 250 250

Then apply those colors to a DEM:

::

    gdaldem color-relief input.tif color_ramp.txt color-relief.tif

**Create slope-shading from a DEM**
First, make a slope raster from DEM:

::

        gdaldem slope input.tif slope.tif 

Second, create a color-slope.txt file:
*(Slope angle, Red, Green, Blue)*

::

    0 255 255 255
    90 0 0 0  

Finally, color the slope raster based on angles in color-slope.txt:

::

    gdaldem color-relief slope.tif color-slope.txt slopeshade.tif

**Resample (resize) raster**

::

    gdalwarp -ts <width> <height> -r cubicspline dem.tif resampled_dem.tif

Entering 0 for either width or height guesses based on current
dimensions.

**Burn vector into raster**

::

    gdal_rasterize -b 1 -i -burn -32678 -l layername input.shp input.tif

**Create contours from DEM**

::

    gdal_contour -a elev -i 50 input_dem.tif output_contours.shp

**Get values for a specific location in a raster**

::

    gdallocationinfo -xml -wgs84 input.tif <lon> <lat>  

Other
-----

**Convert KML points to CSV (simple)**

::

    ogr2ogr -f CSV output.csv input.kmz -lco GEOMETRY=AS_XY

**Convert KML to CSV (WKT)**
First list layers in the KML file

::

    ogrinfo -so input.kml

Convert the desired KML layer to CSV

::

    ogr2ogr -f CSV output.csv input.kml -sql "select *,OGR_GEOM_WKT from some_kml_layer"

**CSV points to SHP**
*This section needs retooling*
Given input.csv

::

    lon_column,lat_column,value
    -81,32,13
    -81,32,14
    -81,32,15

Make a .dbf table for ogr2ogr to work with from input.csv

::

    ogr2ogr -f "ESRI Shapefile" input.dbf input.csv

Use a text editor to create a .vrt file in the same directory as
input.csv and input.dbf. This file holds the parameters for building a
full shapefile based on values in the DBF you just made.

::

    <OGRVRTDataSource>
      <OGRVRTLayer name="output_file_name">
        <SrcDataSource relativeToVRT="1">./</SrcDataSource>
        <SrcLayer>input</SrcLayer>
        <GeometryType>wkbPoint</GeometryType>
        <LayerSRS>WGS84</LayerSRS>
        <GeometryField encoding="PointFromColumns" x="lon_column" y="lat_column"/>
      </OGRVRTLayer>
    </OGRVRTDataSource>

Create shapefile based on parameters listed in the .vrt

::

    mkdir shp
    ogr2ogr -f "ESRI Shapefile" shp/ inputfile.vrt

The VRT file can be modified to give a new output shapefile name,
reference a different coordinate system (LayerSRS), or pull coordinates
from different columns.

**MODIS operations**

First, download relevant .hdf tiles from the MODIS ftp site:
ftp://ladsftp.nascom.nasa.gov/; use the `MODIS sinusoidal
grid <http://www.geohealth.ou.edu/modis_v5/modis.shtml>`__ for
reference.

Create a file containing the names of all .hdf files in the directory

::

    ls -1 *.hdf > files.txt

List MODIS Subdatasets in a given HDF (conf. the `MODIS products
table <https://lpdaac.usgs.gov/products/modis_products_table/>`__)

::

    gdalinfo longFileName.hdf | grep SUBDATASET

Make TIFs from each file in list; replace 'MOD12Q1:Land\_Cover\_Type\_1'
with desired Subdataset name

::

    mkdir output
    cat files.txt | xargs -I % -n1 gdalwarp -of GTiff 
        'HDF4_EOS:EOS_GRID:%:MOD12Q1:Land_Cover_Type_1' output/%.tif

Merge all .tifs in output directory into single file

::

    cd output
    gdal_merge.py -o Merged_Landcover.tif *.tif


**BASH functions**

*Size Functions*

This size function echos the pixel dimensions of a given file in the format expected by gdalwarp.

::

    function gdal_size() {
        SIZE=$(gdalinfo $1 |\
            grep 'Size is ' |\
            cut -d\   -f3-4 |\
            sed 's/,//g')
        echo -n "$SIZE"
    }

This can be used to easily resample one raster to the dimensions of
another:

::

    gdalwarp -ts $(gdal_size bigraster.tif) -r cubicspline 
        smallraster.tif resampled_smallraster.tif

*Extent Functions*

These extent functions echo the extent of the given file in the
order/format expected by gdal\_translate -projwin. (Originally from
`Linfiniti <http://linfiniti.com/2009/09/clipping-rasters-with-gdal-using-polygons/>`__).

::

    function gdal_extent() {
        if [ -z "$1" ]; then 
            echo "Missing arguments. Syntax:"
            echo "  gdal_extent <input_raster>"
            return
        fi
        EXTENT=$(gdalinfo $1 |\
            grep "Upper Left\|Lower Right" |\
            sed "s/Upper Left  //g;s/Lower Right //g;s/).*//g" |\
            tr "\n" " " |\
            sed 's/ *$//g' |\
            tr -d "[(,]")
        echo -n "$EXTENT"
    }

    function ogr_extent() {
        if [ -z "$1" ]; then 
            echo "Missing arguments. Syntax:"
            echo "  ogr_extent <input_vector>"
            return
        fi
        EXTENT=$(ogrinfo -al -so $1 |\
            grep Extent |\
            sed 's/Extent: //g' |\
            sed 's/(//g' |\
            sed 's/)//g' |\
            sed 's/ - /, /g')
        EXTENT=`echo $EXTENT | awk -F ',' '{print $1 " " $4 " " $3 " " $2}'`
        echo -n "$EXTENT"
    }

    function ogr_layer_extent() {
        if [ -z "$2" ]; then 
            echo "Missing arguments. Syntax:"
            echo "  ogr_extent <input_vector> <layer_name>"
            return
        fi
        EXTENT=$(ogrinfo -so $1 $2 |\
            grep Extent |\
            sed 's/Extent: //g' |\
            sed 's/(//g' |\
            sed 's/)//g' |\
            sed 's/ - /, /g')
        EXTENT=`echo $EXTENT | awk -F ',' '{print $1 " " $4 " " $3 " " $2}'`
        echo -n "$EXTENT"
    }

Extents can be passed directly into a gdal\_translate command like so:

::

    gdal_translate -projwin $(ogr_extent boundingbox.shp) input.tif clipped_output.tif

or

::

    gdal_translate -projwin $(gdal_extent target_crop.tif) input.tif clipped_output.tif

This can be a useful way to quickly crop one raster to the same extent
as another. Add these to your ~/.bash\_profile file for easy terminal
access.

********************
Sources
********************

http://live.osgeo.org/en/quickstart/gdal_quickstart.html

https://github.com/nvkelso/geo-how-to/wiki/OGR-to-reproject,-modify-Shapefiles

ftp://ftp.remotesensing.org/gdal/presentations/OpenSource_Weds_Andre_CUGOS.pdf

http://developmentseed.org/blog/2009/jul/30/using-open-source-tools-make-elevation-maps-afghanistan-and-pakistan/

http://linfiniti.com/2010/12/a-workflow-for-creating-beautiful-relief-shaded-dems-using-gdal/

http://linfiniti.com/2009/09/clipping-rasters-with-gdal-using-polygons/

http://nautilus.baruch.sc.edu/twiki_dmcc/bin/view/Main/OGR_example

http://www.gdal.org/frmt_hdf4.html

http://planetflux.adamwilson.us/2010/06/modis-processing-with-r-gdal-and-nco.html

http://trac.osgeo.org/gdal/wiki/FAQRaster

http://www.mikejcorey.com/wordpress/2011/02/05/tutorial-create-beautiful-hillshade-maps-from-digital-elevation-models-with-gdal-and-mapnik/

http://dirkraffel.com/2011/07/05/best-way-to-merge-color-relief-with-shaded-relief-map/

http://gfoss.blogspot.com/2008/06/gdal-raster-data-tips-and-tricks.html

http://osgeo-org.1560.x6.nabble.com/gdal-dev-Dissolve-shapefile-using-GDAL-OGR-td5036930.html

https://www.mapbox.com/tilemill/docs/guides/terrain-data/

https://gist.github.com/ashaw/0862ec044c45b9aa3c76

https://github.com/gina-alaska/dans-gdal-scripts
