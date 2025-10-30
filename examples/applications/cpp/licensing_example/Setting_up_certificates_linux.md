There are quick instructions on how to setup a GPG signed .so so you can test the example.

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

GPG keys can be published to a keyserver and then imported on a different machine. More info on [keys.openpgp.org](https://keys.openpgp.org/about/) which is an freely available keyserver for GPG keys.

To export to a keyserver:

```
gpg --keyserver hkps://keys.openpgp.org --send-keys KEY_ID
```

where `KEY_ID` can be the name or the fingerprint.

To import:

```
gpg --recv-keys KEY_ID
```

You can also export it to a file:

```
gpg --export --armor KEY_ID > public_key.asc
```

and then import it on another machine:

```
gpg --import public_key.asc
```