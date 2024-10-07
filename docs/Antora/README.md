Blueberry documentation
=======================

This directory contains files required to build the official openDAQ
documentation. To build the documentation locally, execute the following Docker
command in the root folder of this repository:

```
docker run -v ${PWD}:/antora:Z --rm -t antora/antora antora-playbook.yml
```

The documentation is generated into the `build/site/` directory.

Building on a local machine
===================
Install Node.js

Install the required NPM packages (remove `-g` to install on a per-user basis).
On Windows you will need to have Administrator permissions to install globally.
* `npm i -g antora`
* `npm i -g @asciidoctor/tabs`
* `npm i -g @springio/antora-extensions`

Then in the repository root run
`npx antora antora-playbook.yml`