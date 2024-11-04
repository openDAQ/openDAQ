openDAQ documentation
=======================

Install Node.js

Install the required NPM packages (remove `-g` to install on a per-user basis).
On Windows you will need to have Administrator permissions to install globally.
* `npm i -g antora`
* `npm i -g @asciidoctor/tabs`
* `npm i -g @springio/antora-extensions`

Then in the repository root run
`npx antora antora-playbook.yml`

The documentation is generated into the `build/site/` directory.