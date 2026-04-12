# a:: Static Documentation

This directory contains a static HTML documentation site for the `a/` subrepo and public `a::` API.

## Language

- Default language: English (`a/docs/*.html`)
- Ukrainian version: `a/docs/uk/*.html`
- Language switch is available on every documentation page via `🇬🇧` and `🇺🇦` flags.

## Entry Points

- `index.html` - overview and navigation hub
- `modules.html` - core + crypto module map
- `api-atlas.html` - compact API contract atlas
- `memory-model.html` - memory algebra and lifecycle rules
- contract layer is documented in `api-atlas.html` + `memory-model.html`
- `two-pass-guide.html` - `a::two_pass` protocol guide
- `crypto-guide.html` - crypto layer guide and safety notes
- `recipes.html` - practical usage templates
- `cpp-knowledge.html` - required C++ knowledge (iceberg onboarding for bare-metal)
- `source/index.html` - source reference for each header

## Local Run

Open `a/docs/index.html` in a browser.

Or start a local server:

```powershell
cd a/docs
python -m http.server 8000
```

Then open `http://localhost:8000`.
