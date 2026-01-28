# Problèmes connus

## API Twelve Data

L’API peut ne pas fournir de données pour plusieurs raisons :
- Clé API invalide ou absente
- Limite d’appels atteinte
- Symbole invalide
- Réponse JSON vide ou en erreur

Dans ce cas, aucun message n’est envoyé dans Kafka et l’interface reste vide.

## Interface vide

Une interface vide ne signifie pas une erreur graphique.
Elle indique simplement qu’aucune donnée n’est disponible en entrée.
