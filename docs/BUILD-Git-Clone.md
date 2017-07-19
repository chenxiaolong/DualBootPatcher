# Cloning the Git Repository

The DualBootPatcher git repository has several git submodules. If the repository has not been cloned yet, pass `--recursive` to the `git clone` command to clone both the repo and the submodules.

```sh
git clone --recursive https://github.com/chenxiaolong/DualBootPatcher.git
```

If the repository has been cloned already, simply run the following command to grab all the submodules. This will need to be done after a `git pull` if any of the submodules were updated.

```sh
git submodule update --init --recursive
```
