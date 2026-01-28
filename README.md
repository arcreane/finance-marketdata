# Market Data Streaming Project

Ce projet met en place une chaîne complète de traitement de données financières en temps réel.
Il récupère des données depuis l’API Twelve Data, les transmet via Kafka (Redpanda), puis les
affiche dans une interface graphique en C++ avec Qt. Les données peuvent également être stockées
dans une base PostgreSQL.

Le projet a pour but de démontrer le fonctionnement d’une architecture temps réel utilisée
dans des contextes professionnels (finance, data streaming, systèmes distribués).

## Architecture générale

Twelve Data API  
→ Producer C++  
→ Kafka / Redpanda  
→ Consumer C++  
→ Interface graphique Qt  
→ PostgreSQL

## Technologies utilisées

- C++17
- Qt (GUI)
- Kafka (via Redpanda)
- Docker / Docker Compose
- PostgreSQL
- API Twelve Data
