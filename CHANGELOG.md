# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.2.3] - 2018-11-28

### Bug fixes:

- Fixed broken NALUs in output regression introduced in previous release.

## [1.2.2] - 2018-11-08

### New features:

- Metamix now drops `Registered user data as specified by Rec. ITU-T T.35` SEI NALUs from output source before injecting closed captions.
- Added FBremix program.

## [1.2.1] - 2018-10-16

### Bug fixes

- Fixed out-of-bounds memory reads of AV stream table on stream restarts.

## [1.2.0] - 2018-10-11

### Breaking changes:

- Slightly changed log messages wording.
- `clearcc` virtual input have been renamed to `clear`.
- The `GET /stats` endpoint's `queueSize` field structure has been changed. It now contains an object of capability name -> metadata queue of corresponding kind size.

### New features:

- Added a concept of run-time adjustable configuration options:
  - Added `tsAdjustment` option for shifting metadata in time.
- Added `caps` field to input descriptions in REST API.
- Added `--no-restart` command line option.
- Added `traceparser` utility script.

### Bug fixes:

No bugs found in this release cycle :tada:

It has been found that Metamix breaks when inputs contain B-frames. Because B-frames do not make sense in live streaming, suggested solution is to put transcoders that remove B-frames in front of Metamix.

### Other changes:

Following changes have been made as part of incoming Ad Markers project, they are included in code base but turned off wherever possible:

- Added a concept of stream classification.
- Added a concept of metadata kind, in this release only one is defined: `closedCaption`.
- Added a concept of stream capabilities.
- There are now multiple metadata queues, one for each metadata kind.
- Extractor and injector codes have been modularized.
- There is no global current input, but separate one for each metadata kind.
  - The syntax of the `POST /input/current` endpoint has been extended in backwards-compatible manner to accomodate to this change.
- Added SCTE-35 parsing & emitting code.

## [1.1.0] - 2018-09-03

### Breaking changes:

- Now Metamix starts by default with `clearcc` input instead of first user-defined input

### New features:

- Added _Virtual Inputs_ concept and implemented `clearcc` virtual input, that constantly emits CC Reset SEIs
- Added `/input/restart` API endpoint that forces input extractor restarts
- Added ability to filter logs by thread with `--log-thread` CLI option
- Added `--version`/`-v` CLI flag
- REST API now includes `isVirtual` field in input info, telling whether this input is virtual or user-defined

### Bug fixes:

- Extractor threads now restart when they detect defects in inputs, this is usually a sign of stream reset, detected defects:
  - non-monotonic DTSes
  - PTS < DTS
- Extractor threads now clean up metadata queue from emitted items on restart
- Fixed some null dereferencing errors on stream failures

### Other:

- Dropped `naludrop` app, using cool video test source in demo instead
- Log messages have been changed (they are shorter yet more informative in some places now)
- Refactored metadata queue implementation

## [1.0.0] - 2018-08-10

This is the very first version of Metamix :tada:
