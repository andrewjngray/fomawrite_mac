# Why build our own writing tool?

Source: [DHH and Lex Fridman — Secret to 10x productivity with AI agents](https://youtu.be/GQjVGFYei9M), published by Lex Clips. Summary based on the complete English (`en`) caption track retrieved on 9 September 2026. Captions contain transcription errors; timing is approximate.

- **0:32–2:02 — Coordination remains a bottleneck.** DHH argues that communication, approvals and management layers can absorb the gains from faster implementation. Direct interaction between a person with product judgment and an agent shortens that loop.
- **2:02–3:15 — Taste and direction matter.** More coding capacity does not automatically produce better software when teams do not know what to improve.
- **3:15–7:05 — Existing organisations adapt slowly.** They discuss AI's rapid development and the opportunity for small teams to challenge established tools and platforms. These are the speakers' arguments, not measured productivity guarantees.
- **7:05–7:48 — Build your own subset.** Recreating the functionality one person needs is a smaller challenge than matching an entire mature product.
- **7:48–9:15 — The writing-app example.** DHH describes moving from iA Writer on Mac to Typora on Linux, then using an agent to make a C++/Qt editor. He reports a first version in roughly 20 minutes and switching after two days of refinement. The final caption names iA Writer inconsistently; the linked Omawrite repository establishes our code source.

Our application: get a dependable editor running, write real documents in it, and let observed friction decide the next feature.
