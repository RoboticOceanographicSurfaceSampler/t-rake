# Data Acquisition system for Oregon State Ice Ocean T-Rake
## Data Acquisition File Format

The t-rake temperature data acquisition system is a 16-channel Analog-to-Digital converter (A/D) with 16-bit resolution per channel and a high-throughput capability, allowing for high-speed high-resolution data capture.  This document discusses how the acquired data is stored on file.

## Configuration File

The configuration file will be installed at

[TBD]

It will be named trakeconfig.json.  Its format will follow this pattern:

```json
{
    "dataPath": "/trake/data",
}
```

## Data File

The data file name will be based on the time stamp at the moment data acquisition is started, and will be of the form

`yyyy-mm-dd_hh.mm.ss.csv`

Multiple files will be stored in the configured data path, and will not collide, since they will all have unique file names.

## Data File Format

The data stored in the aqcuisition files will be ASCII numeric, stored as comma-separated variable (CSV) files.  They will contain a time tick, which is the offset in milliseconds since the file was started, plus a column for each channel.  All channel data will be raw, with no temperature conversions applied.

Example,

```csv
tick,Channel0,Channel1,Channel2,Channel3,Channel4,Channel5,Channel6,Channel7,Channel8,Channel9,Channel10,Channel11,Channel12,Channel13,Channel14,Channel15
1030,48032,6790,8237,908,25780,8004,6078,23985,3440,9605,9640,990,34890,3892,9482,1845
```

The CSV format allows for easy importing into spreadsheets and databases for further processing.
