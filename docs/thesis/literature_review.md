# Literature Review

> **Citation notice (read before submission).** The bracketed markers below follow IEEE style. Entries in the reference list marked **[VERIFY]** are real, well-known works whose *author/title/venue/year are accurate to the best of available knowledge, but whose volume/issue/page numbers you must confirm against the original source*. Entries marked **[PLACEHOLDER]** are slots where you must insert a real peer-reviewed source on the stated topic — do **not** submit them as written. Do not fabricate bibliographic details; committees verify references.

---

## 2.1 Introduction

Fire remains among the most destructive and least predictable hazards in the built environment, and the engineering response to it has historically lagged the threat it confronts. National incident statistics continue to record hundreds of thousands of structural fires annually, with the majority of fatal residential events occurring in premises where detection was absent, unmaintained, or improperly located [1]. The central engineering tension is not the *existence* of detection technology but its *reliability envelope*: legacy architectures are forced to trade sensitivity against specificity, accepting either elevated false-alarm rates or delayed detection. This review surveys the evolution of fire detection and suppression across four technological generations, examines the supporting literature on Internet-of-Things (IoT) telemetry, multi-modal sensor fusion, edge-accelerated computer vision, and occupancy-aware evacuation, and synthesises the residual research gap that the Smart Fire Fighting System (SFFS) is designed to close. Throughout, emphasis is placed on the quantitative parameters — filter dynamics, threshold semantics, inference latency, and communication resilience — that distinguish a deployable life-safety system from a laboratory demonstration.

## 2.2 Historical Evolution of Fire Detection and Suppression

The development of automatic fire protection can be partitioned into four broadly recognised technological generations, each defined by the dominant sensing and actuation paradigm of its era [VERIFY:3].

**Generation 1 — Mechanical thermal suppression.** The earliest automatic systems coupled detection and suppression in a single passive element: the fusible-link sprinkler, which discharges when ambient heat melts a eutectic alloy or ruptures a glass bulb. These systems are mechanically robust and require no electrical power, but they are fundamentally *reactive* and *context-blind*. Actuation requires the fire plume to raise the local ceiling temperature to the link's rating, which — under the canonical *t²* fire-growth model, where heat-release rate scales with the square of time from ignition — implies a detection latency measured in minutes for slow-growing fires [2]. Furthermore, discharge is purely local to the activated head and carries no semantic information about location, severity, or occupancy.

**Generation 2 — Electrified point detection.** The introduction of discrete smoke and heat detectors wired to a central zonal panel decoupled detection from suppression and introduced the concept of building *zones*. Ionisation and photoelectric smoke detectors substantially reduced detection latency relative to thermal links. However, this generation introduced the defining failure mode of modern fire alarms: the *false-positive problem*. Single-variable threshold detectors cannot distinguish combustion aerosols from nuisance sources such as cooking vapour, steam, or airborne dust, producing false-alarm rates high enough to erode occupant trust and provoke system disablement [VERIFY:6].

**Generation 3 — Digital addressable networks.** Addressable systems assigned each detector a unique digital identity on a shared communication loop, enabling per-device status reporting, drift compensation, and software-adjustable sensitivity thresholds [VERIFY:4]. This improved spatial resolution and maintainability, yet the underlying detection logic remained predominantly *single-variable* and *threshold-driven*. Without cross-modal corroboration or visual confirmation, Generation 3 systems continued to face the irreducible trade-off between false positives and missed detections, particularly in heterogeneous multi-room structures where a single global sensitivity setting is necessarily a compromise [VERIFY:6].

**Generation 4 — Multi-modal edge intelligence.** The contemporary paradigm, into which the SFFS is positioned, is defined by three departures from its predecessors: (i) *multi-modal sensor fusion*, combining chemical (gas/smoke), radiative (flame), environmental (temperature/humidity), and optical (computer-vision) modalities so that no single nuisance source can independently trigger suppression; (ii) *hardware-accelerated local inference*, placing deep-learning perception at the network edge rather than in the cloud; and (iii) *fail-operational autonomy*, guaranteeing that the safety-critical suppression loop closes within a single local control cycle independent of wide-area connectivity [VERIFY:5]. This generation reframes the design objective from *alerting* to *deterministic, zone-localised intervention*.

## 2.3 IoT, Sensor Fusion, and Telemetry Protocols in Building Safety

### 2.3.1 Embedded controllers and the multi-species gas matrix

The proliferation of low-cost, Wi-Fi-enabled microcontrollers — the ESP32 class being representative — has made distributed embedded sensing economically viable for building safety [VERIFY:8]. A recurring architectural constraint documented in the literature, and directly relevant to the SFFS, is the contention between the controller's radio subsystem and its analogue-to-digital converters: on the ESP32, ADC2 is multiplexed with Wi-Fi operation, mandating that analogue gas channels be assigned exclusively to ADC1 to preserve sampling integrity during transmission [VERIFY:8]. The SFFS adopts this constraint directly, routing its four metal-oxide (MQ-series) gas sensors to ADC1 channels.

Multi-species chemical sensing is motivated by the differentiated hazard profile of a structure fire. Carbon monoxide is the dominant lethal agent in residential fire fatalities, justifying a dedicated CO channel (MQ-7); flammable hydrocarbons such as LPG and methane present a deflagration risk that warrants early valve isolation (MQ-5/MQ-6); and general combustion aerosols are captured by a broad-spectrum smoke sensor (MQ-2). Because raw metal-oxide sensor outputs are noisy and exhibit baseline drift, the literature consistently recommends temporal smoothing. The SFFS applies an exponential moving-average (EMA) filter of the form *yₙ = α·xₙ + (1−α)·yₙ₋₁* with a low smoothing coefficient (α = 0.15), which attenuates high-frequency transients while preserving the monotonic rise characteristic of a genuine gas accumulation event. A warm-up gate (~60 s) further suppresses the well-documented power-on transient of heated metal-oxide elements, during which readings are not yet trustworthy. Detection is performed against a fixed threshold expressed directly in the 12-bit ADC domain (0–4095) rather than converted to parts-per-million, a deliberate engineering simplification that avoids the per-sensor calibration curve while remaining sufficient for relative-change detection.

### 2.3.2 Telemetry protocols, latency, and network resilience

For real-time building telemetry, the publish/subscribe messaging pattern has become the de facto standard, with MQTT the dominant protocol owing to its minimal header overhead and quality-of-service guarantees [VERIFY:10]. A lightweight broker (e.g., Mosquitto) decouples publishers from subscribers, allowing the embedded tier, the perception tier, and the operator dashboard to interoperate without direct coupling. Two protocol features are of particular importance to life-safety applications. First, *retained messages* and QoS-1 delivery ensure that the most recent system-state command persists for late-joining subscribers, so a reconnecting actuator immediately receives the current safety state. Second, the *Last Will and Testament* (LWT) mechanism allows the broker to publish a predetermined "offline" notification on behalf of a client that disconnects ungracefully, providing deterministic failure detection rather than silent loss. The SFFS exploits both features, complementing them with a periodic heartbeat and exponential-backoff reconnection.

A critical and under-emphasised finding in the applied literature is that telemetry resilience is *coupled to compute headroom*. When the perception tier is compute-bound — for example, when vision inference saturates a CPU — background communication threads experience scheduling starvation, manifesting as broker timeouts and spurious "offline" transitions of otherwise healthy nodes. This observation motivates the SFFS design principle that the deterministic suppression loop must never depend on the network: the embedded controller evaluates local sensor predicates and actuates autonomously, treating any AI- or dashboard-originated command as an *additive* override rather than a prerequisite. Visual dashboards built on flow-based programming environments such as Node-RED provide the human-machine interface layer, subscribing to telemetry topics and rendering per-zone status, environmental trends, and manual-override controls [PLACEHOLDER:11].

## 2.4 Computer Vision and Edge AI for Visual Fire Verification

### 2.4.1 Deep object detection and the YOLO family

The principal weakness of purely chemical detection — its inability to *confirm* a hazard visually — is addressed in the modern literature through convolutional object detection. The single-stage detector paradigm introduced by the "You Only Look Once" (YOLO) architecture reframed detection as a single regression over a spatial grid, achieving real-time inference by eliminating the region-proposal stage of earlier two-stage detectors [12]. Successive iterations improved the speed/accuracy frontier through architectural refinements such as cross-stage partial connections, path-aggregation necks, and anchor-free heads [13], [VERIFY:14]. For fire and smoke, visual detection is attractive because flame and smoke present distinctive chromatic, textural, and temporal signatures; however, the same classes are notoriously prone to *false positives* from fire-coloured but benign stimuli (incandescent lighting, reflections, sunlit surfaces), which is why visual detection in a life-safety system is best deployed as a *corroborating* modality within a fusion rule rather than as a sole trigger.

The SFFS employs a custom-trained YOLOv11-nano model and addresses the false-positive problem through *asymmetric class-specific confidence gating*: the acceptance threshold for the visually ambiguous *smoke* class is set substantially higher (0.82) than that for *fire* (0.68), reflecting the higher base rate of smoke-like distractors. This thresholding policy is the perception-tier analogue of the chemical-tier EMA filter — both raise the evidentiary bar required before a detection is admitted into the fusion logic.

### 2.4.2 Edge acceleration, mixed precision, and frame-skipping

Deploying object detection at the edge imposes a hard real-time constraint that the literature characterises through the trade-off between inference latency and detection cadence. Half-precision (FP16) inference on a CUDA-capable GPU roughly halves memory bandwidth and exploits dedicated tensor hardware, yielding substantial throughput gains over full precision with negligible accuracy loss for inference workloads [VERIFY:16]. Empirically, the SFFS sustains real-time perception on a mobile-class GPU (≈23 frames/s under FP16) versus a markedly lower CPU throughput (≈7 frames/s).

When GPU acceleration is unavailable, the canonical mitigation is *temporal decimation*: running the expensive detector on a subsampled frame cadence (e.g., every third frame) while retaining lightweight per-frame operations. The critical design subtlety, documented in tracking literature, is that decimating the *detector* must not decimate the *tracker* — identity association must run every frame to preserve persistent track IDs, otherwise low effective frame rates induce identity switches and track fragmentation. The SFFS adopts exactly this separation, throttling fire inference under CPU operation while maintaining per-frame person tracking.

## 2.5 Occupancy-Aware Tracking and Smart Evacuation

A decisive limitation of legacy fire systems is their complete absence of *occupancy awareness*: responders are given no estimate of how many people remain inside a hazard zone or where they are located. The computer-vision literature offers mature primitives for closing this gap. Multi-object tracking-by-detection frameworks such as Simple Online and Realtime Tracking (SORT) associate detections across frames using a Kalman-filter motion model and Hungarian-algorithm assignment [17], and its deep-association extension (DeepSORT) augments this with an appearance embedding to reduce identity switches through occlusion [18]. Persistent identity is the enabling property for headcount estimation.

For interior occupancy counting, the *line-crossing* method is widely adopted for its computational economy and robustness: a virtual boundary is defined in the image plane, and a person's transit is counted when the trajectory of a tracked centroid crosses the line, with the direction of crossing distinguishing entries from exits [PLACEHOLDER:19]. The SFFS implements this directly, maintaining a signed occupancy count from directional crossings of a fixed vertical boundary, with the count floored at zero to prevent negative drift. This count is propagated into the alerting payload so that responders receive a live estimate of trapped occupants — the input required for rescue-path prioritisation and, in proposed extensions, dynamic evacuation routing [PLACEHOLDER:20]. The literature frames such occupancy estimates as the foundational layer upon which higher-order capabilities (named identification, predictive egress modelling) can subsequently be built, while noting the privacy and compute obligations that identification specifically introduces.

## 2.6 Critical Synthesis and Research Gap

Synthesising the surveyed literature reveals a consistent set of unresolved limitations in deployed fire-protection systems:

1. **The false-alarm/missed-detection trade-off.** Single-variable, threshold-driven detection — characteristic of Generations 2 and 3 — cannot simultaneously minimise false positives and detection latency, and the resulting nuisance alarms actively degrade safety through system distrust and disablement [VERIFY:6].

2. **Untargeted suppression.** Conventional systems lack zonal semantics; a single trigger applies suppression building-wide, inflicting collateral water damage on electronics and assets in unaffected compartments [VERIFY:6].

3. **Connectivity-coupled reliability.** Cloud- or network-dependent "smart" systems forfeit their safety function precisely when infrastructure is most likely to fail — during structural compromise or network outage [VERIFY:5].

4. **Absent occupancy awareness.** Legacy systems convey no information on the number or location of trapped occupants, depriving responders of the data most relevant to life safety [PLACEHOLDER:20].

The SFFS is positioned as a direct, integrated response to each of these gaps, and — critically — its claims are bounded by what is physically implemented in the prototype rather than by aspirational scope:

- Against **(1)**, it replaces single-variable thresholding with *multi-modal fusion* across chemical (MQ-2/5/6/7), radiative (IR flame), environmental (DHT22), and optical (YOLOv11) modalities, each gated by its own evidentiary filter (EMA smoothing and warm-up on the chemical tier; asymmetric confidence thresholds on the optical tier), so that no single nuisance source independently commands suppression.
- Against **(2)**, it implements *deterministic four-zone isolation*, mapping each compartment to an independently actuated pump and barrier set so that suppression is confined to the affected room.
- Against **(3)**, it enforces *fail-operational edge autonomy*: the ESP32 evaluates local predicates and actuates within a single control cycle, treating MQTT-borne AI verdicts as additive overrides, so that loss of the network or perception tier never disables suppression.
- Against **(4)**, it integrates *line-crossing occupancy estimation* and embeds the live count in the remote alert, supplying responders with actionable rescue-prioritisation data.

In aggregate, the SFFS instantiates the Generation-4 paradigm not as a single innovation but as the disciplined integration of established techniques — pub/sub telemetry, edge-accelerated detection, tracking-by-detection occupancy counting, and zonal actuation — under the governing constraint of deterministic, network-independent life safety. It is this integration, rather than any individual component, that constitutes the contribution positioned against the surveyed literature.

---

## References

> *Replace every **[PLACEHOLDER]** with a verified peer-reviewed source. Confirm volume/issue/page fields for every **[VERIFY]** entry before submission.*

[1] National Fire Protection Association (NFPA), *Fire Loss in the United States* (annual report series), NFPA, Quincy, MA. **[VERIFY — cite the specific year/edition you quote in Ch. 1.]**

[2] D. Drysdale, *An Introduction to Fire Dynamics*, 3rd ed. Chichester, U.K.: Wiley, 2011. **[VERIFY — supports the t² fire-growth model.]**

[3] *Survey of the generational evolution of building fire-alarm architectures.* **[PLACEHOLDER — insert a peer-reviewed survey of fire-detection system generations.]**

[4] *Digital addressable fire-detection networks and adjustable sensor thresholds.* **[PLACEHOLDER — insert a real source on addressable detection systems.]**

[5] *Fail-operational local control loops in network-disconnected IoT systems.* **[PLACEHOLDER — insert a real source on edge autonomy / fail-operational control.]**

[6] *The false-alarm versus missed-detection balance in fire detection.* **[PLACEHOLDER — insert a real source on fire-alarm false-positive rates.]**

[7] National Fire Protection Association, *NFPA 72: National Fire Alarm and Signaling Code*, NFPA, Quincy, MA. **[VERIFY — cite the edition referenced.]**

[8] Espressif Systems, *ESP32 Technical Reference Manual*, Espressif, 2023 (and *ESP32 Datasheet*). **[VERIFY — supports the ADC1/Wi-Fi contention constraint; confirm version.]**

[9] *Exponential moving-average / digital low-pass filtering for noisy sensor signals.* **[PLACEHOLDER — insert a signal-processing reference for the EMA filter.]**

[10] A. Banks and R. Gupta, Eds., *MQTT Version 3.1.1*, OASIS Standard, Oct. 2014. **[VERIFY — confirm exact citation/URL.]**

[11] *Flow-based programming for IoT dashboards (Node-RED).* **[PLACEHOLDER — insert a reference for Node-RED / flow-based IoT visualization.]**

[12] J. Redmon, S. Divvala, R. Girshick, and A. Farhadi, "You Only Look Once: Unified, Real-Time Object Detection," in *Proc. IEEE Conf. Computer Vision and Pattern Recognition (CVPR)*, 2016, pp. 779–788. **[VERIFY page range.]**

[13] A. Bochkovskiy, C.-Y. Wang, and H.-Y. M. Liao, "YOLOv4: Optimal Speed and Accuracy of Object Detection," *arXiv:2004.10934*, 2020. **[VERIFY.]**

[14] G. Jocher *et al.*, "Ultralytics YOLO" (software), Ultralytics. **[VERIFY — cite the YOLOv11 release/version you used.]**

[15] *Survey of CNN-based fire and smoke detection.* **[PLACEHOLDER — insert a peer-reviewed survey of deep-learning fire detection.]**

[16] P. Micikevicius *et al.*, "Mixed Precision Training," in *Proc. Int. Conf. Learning Representations (ICLR)*, 2018. **[VERIFY — supports FP16 inference.]**

[17] A. Bewley, Z. Ge, L. Ott, F. Ramos, and B. Upcroft, "Simple Online and Realtime Tracking," in *Proc. IEEE Int. Conf. Image Processing (ICIP)*, 2016, pp. 3464–3468. **[VERIFY page range.]**

[18] N. Wojke, A. Bewley, and D. Paulus, "Simple Online and Realtime Tracking with a Deep Association Metric," in *Proc. IEEE Int. Conf. Image Processing (ICIP)*, 2017, pp. 3645–3649. **[VERIFY page range.]**

[19] *Line-crossing / virtual-boundary people counting.* **[PLACEHOLDER — insert a real source on line-crossing occupancy counting.]**

[20] *Occupancy awareness and evacuation routing for emergency response.* **[PLACEHOLDER — insert a real source; you may reuse the occupancy/evacuation references from your existing list if verified.]**
