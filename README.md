# slss: Secure Large Secret Sharing

This utility combines Gallois Field Matrix based Reed-Solomon encoding
and an all-or-nothing transform to provide secure sharing of large secrets.

The secret is split into a specified number of files, a given number of which
are required to recover the secret. Access to fewer then the required number
of files for recovery cannot (see "Schneier's Law") reveal any plaintext.

All recovery files contain the source to this utility allowing it to be
re-built if needed for recovery.

To split a secret execute the utility specifying the filename stub,
number of shares to create and number of shares required to recover:

    $ slss my_big_secret_file 6 3
    splitting "my_big_secret_file" into 6 shares named "my_big_secret_file_xx.tar",
        3 of which are needed to recover
    $ ls
    my_big_secret_file         my_big_secret_file_01.tar  my_big_secret_file_03.tar
    my_big_secret_file_05.tar  my_big_secret_file_00.tar  my_big_secret_file_02.tar
    my_big_secret_file_04.tar  my_big_secret_file.sha256

Note the following files:

 - my\_big\_secret\_file : The original secret
 - my\_big\_secret\_file_xx.tar : the shares
 - my\_big\_secret\_file.sha256 : sha256 checksums of the shares and
 all-or-nothing encrypted secret

The 6 secret shares are then distributed. The shecksum file should be saved to
enable verification of the shares.

To recover the secret gather the required number of shares and run the utility
in recovery mode

    # recover using 3 of the 6 shares:
    $ ls
    my_big_secret_file_01.tar  my_big_secret_file_05.tar
    my_big_secret_file_04.tar  my_big_secret_file.sha256

    # optionally verify the presented shares
    $ sha256sum --check --ignore-missing my_big_secret_file.sha256
    my_big_secret_file_01.tar: OK
    my_big_secret_file_04.tar: OK
    my_big_secret_file_05.tar: OK

    # recover
    $ slss my_big_secret_file
    gfm(3, 3).failData(0)
    gfm(3, 3).failData(2)
    gfm(3, 3).failData(3)

    # see that the original secret has been recovered
    $ ls
    my_big_secret_file         my_big_secret_file_04.tar  my_big_secret_file_aont
    my_big_secret_file_01.tar  my_big_secret_file_05.tar  my_big_secret_file.sha256


To recover the recovery tool extract the nested source tarball and build it:

    # files, as before
    $ ls
    my_big_secret_file_01.tar  my_big_secret_file_05.tar
    my_big_secret_file_04.tar  my_big_secret_file.sha256

    # extract the nested tarball
    $ tar --extract --file my_big_secret_file_04.tar
    $ tar --extract --file slss.tar.xz

    # re-build the recovery tool
    $ ls
    my_big_secret_file_01.tar  my_big_secret_file.sha256         slss.tar.xz
    my_big_secret_file_04.tar  slss-0.0.1-0-ga832146-dirty
    my_big_secret_file_05.tar  slss-0.0.1-0-ga832146-dirty.diff
    $ make --directory slss-0.0.1-0-ga832146-dirty
    ...

    # recover using the freshly-built tool
    $ ./slss-0.0.1-0-ga832146-dirty/slss my_big_secret_file
    gfm(3, 3).failData(0)
    gfm(3, 3).failData(2)
    gfm(3, 3).failData(3)
    $ ls
    my_big_secret_file         my_big_secret_file_05.tar  slss-0.0.1-0-ga832146-dirty
    my_big_secret_file_01.tar  my_big_secret_file_aont    slss-0.0.1-0-ga832146-dirty.diff
    my_big_secret_file_04.tar  my_big_secret_file.sha256  slss.tar.xz



This program is based on two papers,
*H. Peter Anvin, 'The mathematics of RAID-6', December 2004*
and
*James S. Plank, [A Tutorial on Reed-Solomon Coding for Fault-Tolerance in
RAID-like Systems](http://www.cs.utk.edu/~plank/plank/papers/CS-96-332.pdf)*
and
*[Note: Correction to the 1997 Tutorial on Reed-Solomon Coding](http://web.eecs.utk.edu/~plank/plank/papers/CS-03-504.pdf)*.
