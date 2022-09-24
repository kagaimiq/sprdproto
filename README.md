# sprdproto

Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs.

## First of all

First of all, i am obviously not the only one who ever tried to talk to these SoCs ever.
Just take them as an example:

- [sharkalaka](https://github.com/fxsheep/sharkalaka)
  - born at **26th August of 2021**
- [UniFlash](https://gitlab.com/suborg/uniflash)
  - born at **18th December of 2021** (the [post](https://chronovir.us/2021/12/18/Opus-Spreadtrum/) and the repo itself, seems like),
  - i'd like to be as such - i wanna put "**Copydown (X) 2022, 水 Mizu-DEC, No rights reserved**" here!!! //it's a fake company btw//
    Unlicense goes on! do whatever you want, i don't care
    uhmm... okay, seems like i've gone too far, where did i stop at...

But! Take attention at the dates when they were born.

Let me tell that the ["notes"](Spreadtroom Protocol.txt) that i took about the protocol were last dated at **27th April of 2021**!
while the **sprdproto** itself might've been written somewhere in that time frame (March-April-May of 2021 for sure).

So i am actually the first one ?!

The only problem is that i rememberred about my GitHub account somewhere in *December of 2021*,
and then i uploaded this thing way later in *1st February of 2022*, when i wasn't the "first", at least in my honest opinion...
But who cares afterall!

## Description

### sprdproto

`./sprdproto <payload 1 addr> <payload 1 file> [<payload 2 addr> <payload 2 file>]`

It tells all by itself.

The first payload is loaded on the BootROM stage, and so, the only thing you have is the internal SRAMs.

The second payload is loaded on the FDL1 stage, which is provided by the first payload, and in that,
you have access to the DRAM.

### sprdrunner.py

Not done yet... but for some insights:

`python3 [--dram] address file`

The `--dram` key is used to ensure that the DRAM is initialized (i.e. it will/should run FDL1).

The `address` and `file` is obviously the load address and the file that will be executed.
