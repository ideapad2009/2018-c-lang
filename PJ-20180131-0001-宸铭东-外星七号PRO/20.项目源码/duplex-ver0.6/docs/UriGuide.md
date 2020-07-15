# API Document
## Setup Media control using URI
- URI shceme `schem://[username:pass@]host[:port][/path][#fragment]`
- `scheme`:
    + `http/https` for http stream reader
    + `i2s` for i2s stream reader
    + `adc` for adc stream reader
    + `file` for file stream reader
    + `raw` for raw stream reader
    + `flsh` for flash stream reader
- `username:pass`
    + for http stream reader username/pass (if present)
    + for i2s/adc sample rates/channel record
- `host`
    + for http host name
    + for i2s/dac filename + extension
- `port`
    + for http port
- `/path`
    + for http path
    + for i2s/dac record output path
- `#fragment`
    + for http output stream (defaut is i2s)
    + for i2s/dac output
- example:
    + `http://server.com/file.mp3#i2s` play to i2s output stream mp3 file from server.com
    + `file:///filename.mp3#i2s` play to i2s output stream mp3 file
    + `i2s://16000:2@record.opus/record.bypass#file` record audio from i2s interface, sample rate 16Khz, 2 channel, using opus encoder, bypass to decoder and save to file://record.opus file
    + `i2s://16000:2@record.bypass/record.bypass#i2s` record audio from i2s interface, sample rate 16Khz, 2 channel, bypass to encoder, bypass to decoder and play to i2s interface
    + `adc://16000:2@record.opus/record.opus#dac` record audio from adc interface, sample rate 16Khz, 2 channel, using opus encoder, using opus decoder playback to dac interface
    + `raw://from.pcm/to.pcm#i2s` play to i2s output stream from raw buffer
## Service
