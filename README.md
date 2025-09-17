# slss: Secure Large Secret Sharing

This utility combines Gallois Field Matrix based Reed-Solomon encoding
and an all-or-nothing transform to provide secure sharing of large secrets.

The secret is split into a specified number of files, a given number of which
are required to recover the secret. Access to fewer then the required number
of files for recovery
[cannot](https://en.wikipedia.org/wiki/Bruce_Schneier#Cryptography "Schneier's law")
reveal any plaintext.

All recovery files contain the source to this utility allowing it to be
re-built if needed for recovery.

## encrypting and splitting a file

To split a secret execute the utility specifying the filename stub,
number of shares to create and number of shares required to recover:

    $ slss my_big_secret_file 6 3
    splitting "my_big_secret_file" into 6 shares named "my_big_secret_file_xx.tar",
        3 of which are needed to recover
    $ ls
    my_big_secret_file         my_big_secret_file_02.tar  my_big_secret_file_05.tar
    my_big_secret_file_00.tar  my_big_secret_file_03.tar  my_big_secret_file.sha256
    my_big_secret_file_01.tar  my_big_secret_file_04.tar

Note the following files:

 - my\_big\_secret\_file : The original secret
 - my\_big\_secret\_file_xx.tar : the shares
 - my\_big\_secret\_file.sha256 : sha256 checksums of the shares and
 all-or-nothing encrypted secret

## encrypting and splitting a stream

The the secret file does not exist slss will take input from STDIN
(this avoids the need to ever write the secret to disk):

    $ rm my_big_secret_file
    $ some_secret_process | slss my_big_secret_file 6 3
    splitting STDIN into 6 shares named "my_big_secret_file_xx.tar",
        3 of which are needed to recover

The 6 secret shares are then distributed. The shecksum file should be saved to
enable verification of the shares.

## recovering the secret

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

    # see that the original secret has been recovered
    $ ls
    my_big_secret_file         my_big_secret_file_04.tar  my_big_secret_file_aont
    my_big_secret_file_01.tar  my_big_secret_file_05.tar  my_big_secret_file.sha256

## recovering slss

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
    my_big_secret_file_01.tar  my_big_secret_file.sha256
    my_big_secret_file_04.tar  slss-<version info>
    my_big_secret_file_05.tar  slss.tar.xz
    $ make --directory slss-*/
    ...

    # recover using the freshly-built tool
    $ ./slss-*/slss my_big_secret_file
    $ ls
    my_big_secret_file         my_big_secret_file_05.tar  slss-<version info>
    my_big_secret_file_01.tar  my_big_secret_file_aont    slss.tar.xz
    my_big_secret_file_04.tar  my_big_secret_file.sha256


This program is based on two papers,
*H. Peter Anvin, 'The mathematics of RAID-6', December 2004*
and
*James S. Plank, [A Tutorial on Reed-Solomon Coding for Fault-Tolerance in
RAID-like Systems](http://www.cs.utk.edu/~plank/plank/papers/CS-96-332.pdf)*
and
*[Note: Correction to the 1997 Tutorial on Reed-Solomon Coding](http://web.eecs.utk.edu/~plank/plank/papers/CS-03-504.pdf)*.
