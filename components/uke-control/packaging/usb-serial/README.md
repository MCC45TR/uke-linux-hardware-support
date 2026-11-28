# USB developer serial teardown

The Uke Control 2.17.2 package stops the owned serial getty and checks local
readers before removing its USB ACM function. It accepts `fuser` status 1 only
when it reports no output. When local readers still hold the TTY, the function
remains available for a later close retry.

The two patches apply in numbered order to the cumulative Control recipe. The
fixture checks quiesce, live-reader refusal and retry on the patched helper.
The complete source recipe and packaged inputs are available in the signed
[COPR build](https://copr.fedorainfracloud.org/coprs/build/11403102/).
