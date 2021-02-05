# BasKetWeaver

The basketweaver is a tool dedicated for developers. It is used to create or 
extract baskets archive files.

# How to use

## Print Help
A list of the command line options can be printed via the `-h`/`--help` 
option.
```
basketweaver --help
```


## Decode a basket archive

In order to decode a `.baskets` file, use the `--unweave`/`-u` option:

```
basketweaver --unweave welcome/Welcome_en_US.baskets --output welcome/ --name extracted
```
This command will extract the archive to `./welcome/extracted`.

## Encode a basket directory

A valid baskets directory structure can be encoded with the `--weave`/`-w` 
option.

```
basketweaver --weave mybasket-source --name mybasketArchive
```

You'll find the `mybasketArchive.baskets` in the parent directory of `mybasket-source`.



