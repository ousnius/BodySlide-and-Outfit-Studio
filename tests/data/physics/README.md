# Physics XML test data

`hdtSMP64.xsd` is the schema shipped with **Faster HDT-SMP (FSMP)**, from
<https://github.com/DaymareOn/hdtSMP64>, GPL-3.0 - the same project and licence as the
physics core ported into `src/physics/hdt/`.

It is checked in so `PhysicsXmlTest` can assert that the schema table in
`src/physics/XmlSchema.cpp` still covers every element and attribute the format defines.
When FSMP publishes a new schema, replace this file and let the coverage test point at
whatever it has gained.

The `*.xml` files beside it are hand-written fixtures, not real mod content.
