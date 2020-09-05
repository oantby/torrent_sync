# My Torrent Syncing Thing

This all started with one simple idea: I've got a set of drives.  I would like them
split geologically, and I would like them to all have the data.  One can be deemed
the "main" which the others follow, but I should be able to switch "main" to a different
one with relatively little effort.

A lot of options were available; `rsync`, `mrsync`, and bittorrent being the most
prominent.  I had an initial goal of minimizing the number of reads from *each*
disk, therefore sharing the load.  That goal somewhat fell off over time, as I
reminded myself and was reminded that reading is seldom the cause of death for
*any* type of HDD, and actually causes modern filesystems to verify their blocks.

That said, I still wanted a system that:

	1. Enabled me to cross geological boundaries without much effort (effectively
	disqualifying `mrsync` and comparable multicast options).
	2. Enabled quick removal/addition of new clients and re-choosing of "main"
	(effectively disqualifying `rsync`)

So, as such, I present: whatever I ended up making in terms of a bittorrent setup.

`torrent_tree` enables creation of a set of torrent files, one per source file,
that retain the original directory structure.  One torrent file per source file
ensures that 1) you can set seeding ratios *per-file* and 2) you can add new files
without the need to build a new torrent file that requires re-validating every
file in the directory.

`transmission_maintenance.py` is not intended to be part of the final product,
but serves its purpose well in the meantime: it requires `config.py`, specifying
a dict named `config`, as laid out in the example file.  When run, it will retrieve
the specified URL listing torrents, retrieve all torrents currently running in
`transmission-daemon` (from JSON RPC), and make the necessary adjustments to make
the latter match the former.  Note: it *will* delete any files that are now-untracked.

`transmission_maintenance` allows you to specify a certificate authority (path)
used to verify server data, as well as a client cert used by the application
to verify itself to the server.  In a future iteration, these certificates may
extend to being used for verifying the torrent clients between the tracker and/or
each other.

## Future things that may appear here

I kind of like the idea of a trust relationship between the client and peer,
and as such may design a bittorrent client and tracker with the tracker providing
a client TLS certificate signed by some "root" cert, such that adding a new server
is simply generating a cert, signing it with the root cert, and starting the client.

If I decide I've got a great method for making the torrents themselves available
and protected without bloating any individual pieces, I'll probably do that.

# Using this thing

If you're into docker, then you can build the included `Dockerfile`, run it, mounting
your files directory into the container, and know you've got a well-compiled version
with the same functionality I've got.

If you're not into docker (as I only use it to enable consistency on some old hardware),
the included `Makefile` really ought to be able to get this thing built on a reasonable
compiler.  The `filesystem` header is required.  This thing has only been tested on
`*nix`, so approach with caution if that's not you.

Once you have a compiled and available program, you can do:
```
torrent_tree /path/to/files output/directory scheme://tracker-uri
```
to begin creation of torrent files for every file (recursively) in `/path/to/files`.
`output/directory` and any subfolders necessary will be created by the process.

Verbosity is enabled by having more than 4 arguments, so tack anything onto the
end of the `torrent_tree` call to enable verbosity, which notes the beginning and
finish of processing of each file.

Cheers.