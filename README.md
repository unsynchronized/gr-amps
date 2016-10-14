# Introduction

gr-amps is a set of blocks that implement many of the features of an AMPS base station.  The included flowgraph (ampsbs.grc) is essentially a BS-in-a-box.

This is a work in progress, and is an experimental/research tool.  Expect occurrences of poor code quality, unreliability, and general brokenness.

It requires IT++ and gnuradio 3.7.x.

# WARNING

Using this to transmit over the air could potentially disrupt legitimate transmissions and/or devices.  It may also be against local laws.  Please be sure you are not interfering with anything; if you're not sure, use only in a shielded environment (or via direct cable connection, without an antenna).

When using this tool, it is **your responsibility** to prevent bad things from happening.

# The Blocks

### AMPS FOCC (forward control channel)

This block generates a stream of Manchester symbols (two symbols per bit) that can be modulated to form a 10k bit/s FOCC.  

FOCC parameters are all hardcoded at the moment; see `focc_impl::make_superframe` in lib/focc_impl.cc to see the layout of the overhead message train.  The parameters it uses are:
- SID = 00016
- DCC = 0
- AUTH = 0
- S (send serial number) = 0
- E (extended address field) = 0
- BIS = 0 (ignore the busy/idle bits)

This configuration is the most straightforward, because it doesn't involve any authentication challenge-response overhead (currently unimplemented) or encryption (likewise).  This also means it's the least secure (by 1990s standards), since the phone's ESN is sent for every request.  

This block contains changeable busy/idle bits; however, the global variable controlling them is currently always set to idle (and never changed by any other block).  This is a work in progress; for testing use, this is just fine since BIS=0 is in effect and therefore the phones ignore it.

### AMPS RECC (reverse control channel)

This block receives a stream of Manchester-encoded symbols (two symbols per bit) and looks for RECC seizures and bits following it that may contain a message.

After a potential message is received, it is sent to the AMPS RECC Decode block for processing.

### AMPS RECC Decode

This block does the work of decoding and analyzing potential RECC messages.  It has limited functionality so far, but as of now it can handle origination (i.e. phone dials a number) and page response messages.  In the case of origination, it routes the MS (via the FOCC block) to channel 356 and sends a page to the dialed address.  In the case of page response, it routes the MS to 355 and instructs the FVC of 355 to alert briefly, so the phone rings.

### AMPS FVC (forward voice channel)

This block generates a stream of Manchester symbols (two symbols per bit) that can be modulated to form a 10k bit/s FVC.

Unlike the FOCC, which transmits data continuously, the FVC operates on a blank-and-burst basis; when the SAT for the channel is transmitted, it's in audio mode.  When the SAT is not present, the MS will listen for FVC data words.  This block only generates the bursts (repeating them over and over); additional logic is required to create a properly-operating FVC.

### AMPS Command Processor

This block takes in PDUs consisting of text-based commands (e.g. from a GR Socket PDU block) and executes them.  Supported commands are:

- `fvc off`: Disables the data on FVC 355 and enables the audio stream
- `fvc on`: Enables the data on FVC 355 and disables the audio stream
- `fvc alert`: Change the message on the AMPS FVC block to an alert order word
- `page NPANNNNNNN`: Pages the given number

# The Flowgraph

The ampsbs.grc flowgraph ties these all together, broadcasting an FOCC on channel 354 (the last control channel for system B).  A correspodning RECC is set up to listen for messages from MSes.

Two FVCs are set up - one on 355 with a corresponding RVC and one on 356.  The same audio stream (played from a wav file) is sent to both of them.  The RECC Decode block sends mobile-originated calls to 356, and calls from pages to 355.

ampsbs.grc targets the USRP (it was developed using an N210); but nothing about the graph or this code is particularly USRP-specific, and other devices may be used.  (The FOCC block has been successfully used with a HackRF sink, for example.)

# Credits

This code was written by Brandon Creighton (<cstone@pobox.com>).

## Shout Outs

- Pinguino, for the socks
- Eliot and Bob, for phones
- cnelson for idea-bouncing
- Ninja Networks

# License

This code is in the public domain. However, be aware that many of its dependencies, including GNU Radio and IT++, are not.

# References

- TIA/EIA-553-A (1999)
- RSS 118, specifically Annex A (https://www.ic.gc.ca/eic/site/smt-gst.nsf/vwapj/rss118annex.PDF/$FILE/rss118annex.PDF)
- Brian Oblivion, "Cellular Telephony".  Phrack 38, file 9 (part 1); and Phrack 40, file 6 (part 2).
- John G. van Bosse & Fabrizio U. Devetak, Signaling in Telecommunication Networks, 2nd ed.
- Andrew Miceli, Wireless Technician's Handbook, 2nd ed. 


