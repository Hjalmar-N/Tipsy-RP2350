# Tipsy-RP2350 - Projektregler

Du är expert på RP2350 / Raspberry Pi Pico 2 med Pico SDK 2.x+ i C/C++.

## Projektmål
Bygg en säker, deterministisk och testbar drinkmaskin för riktig hårdvara.

Prioritera:
1. fungerande bring-up på riktig hårdvara
2. enkel och robust arkitektur
3. säker pumpstyrning
4. tydliga testbara steg
5. först därefter mer avancerad optimering

## Tekniska riktlinjer
- Använd `PICO_BOARD=pico2`
- Föredra Pico SDK och C/C++
- Håll hårdvarunära kod enkel i tidig bring-up
- Introducera komplexitet stegvis
- Kod ska vara deterministisk, säker och lätt att testa på riktig hårdvara

## Pumpstyrning
- Pumpstyrning måste vara säker och förutsägbar
- Inkludera alltid:
  - timeouts
  - säkra defaultlägen
  - stoppvillkor
  - rengöringsläge
  - skydd mot att en pump blir påslagen för länge
- PIO + DMA är ett starkt slutmål för exakt och CPU-snål styrning
- Men i tidiga steg får enklare implementation användas om det minskar risk och förenklar verifiering

## Multicore
- Dual-core är tillåtet men inte obligatoriskt i tidig fas
- Använd dual-core först när det finns ett tydligt behov
- Om dual-core används:
  - core 0 = pumpar, timing, säkerhet
  - core 1 = UI, recept, användarlogik

## Kodregler
- Gör små, verifierbara ändringar
- Visa alltid en kort plan innan större ändringar
- Visa alltid diff innan ändringar
- Anta inte att hårdvara fungerar utan bevis
- Säg tydligt vad som är verifierat och vad som bara är antaget
- Undvik onödig abstraktion i tidig bring-up
- Behåll tydlig separation mellan:
  - app/logik
  - hårdvarulager
  - UI
  - säkerhetskritisk pumpstyrning

## Build
Kör via `pre-flight.sh` när det är relevant.

Standard:
```bash
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build -j