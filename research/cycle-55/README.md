# Cycle 55a: Focus menu grouping (partial)

The captured iA Focus menu places Sentence, Paragraph and Typewriter under Enable Focus Mode. Omawrite now uses that hierarchy while preserving its existing sentence/paragraph exclusivity and independent typewriter scrolling. Writing Review remains an Omawrite-specific action after a separator. This phase makes no Markdown, highlighter, recovery or master-state changes.

`./bin/build` and the full native-access `./bin/test` suite pass with **90 tests, zero failures and zero skips**. The refreshed stable Dev app exposed Focus → Enable Focus Mode → Sentence/Paragraph/Typewriter. Sentence was triggered on a synthetic document and triggered again to restore the prior off state; document source and saved status stayed unchanged. Screenshot capture was unavailable, so dimming, scroll feel, checked marks and narrow/dark/fullscreen visuals remain unverified. [Optional usability exercise](../usability/cycle-55.md).

Live parts-of-speech and style-check overlays are outside 55a. Their intended iA menu labels are captured, but exact rules, color and performance behavior are not; 55b requires a separate bounded implementation and acceptance pass.
