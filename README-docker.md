# Infrastructure Docker

L’infrastructure repose sur Docker et Docker Compose. Elle permet de lancer tous les services
nécessaires au projet sans installation manuelle complexe.

## Services inclus

- Redpanda : broker Kafka
- Redpanda Console : interface web pour Kafka
- PostgreSQL : base de données
- Adminer : interface web pour la base de données

## Lancement

Depuis le dossier contenant le fichier docker-compose.yml :

```bash
docker-compose up -d
