There are quick instructions on how to setup a GPG signed .so so you can test the example, I will improve on them in the future.

First install GPG:

```bash
sudo apt update && sudo apt install gnupg
```

Then you need to setup a GPG private key:

```bash
gpg --full-generate-key
gpg --list-secret-keys
```

You can use option RSA sign (4).

Then you can create a detached signature:

```bash
gpg --armor --detach-sign library.so
```

And that's it. You should now have a `library.so.asc` signature file. Make sure both of these are in the same location (i.e. the `bin/Debug/` folder) and run the `./licensing_example` script. The script should show you the fingerprint of the license. Copy paste it into the key variable and run the script again and it should complete.
