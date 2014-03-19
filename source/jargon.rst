=================
Jargon Glossary
=================

+-------------------+----------------------------------------+
| Term              |    Definition                          |
+===================+========================================+
| Acquisition       | | A digitizer can be designated as an  |
| monitor           | | acquisition monitor board. This tells|
|                   | | the event builder not to include the |
|                   | | digitizer in event building and to   |
|                   | | push the data directly to the file   |
|                   | | builder. You may want to do this for |
|                   | | signals that should always be        |
|                   | | digitized, like a clock, sum waveform|
|                   | | or monitors of other DAQ systems.    |
+-------------------+----------------------------------------+
| Collection        | | In mongodb a collection is something |
|                   | | like a sub-folder of a database.     |
|                   | | Kodiaq creates a new collection for  | 
|                   | | each data-taking run in default mode.|
|                   | | It is also possible to manually      |
|                   | | define a collection via the          |
|                   | | initialization file.                 |
+-------------------+----------------------------------------+
| Document          | | A mongodb data object. In kodiaq a   |
|                   | | document includes the data payload,  |
|                   | | usually from one occurrence, and some|
|                   | | metadata.                            |
+-------------------+----------------------------------------+
| Occurrence        | | Using a custom firmware, our V1724   |
|                   | | digitizers are entirely              |
|                   | | self-triggered. This means that when |
|                   | | there is only baseline on a channel  |
|                   | | it is not digitized, only a short    |
|                   | | window around above-threshold signals|
|                   | | is digitized. This short window is   |
|                   | | called an occurrence.                |
+-------------------+----------------------------------------+
| Run control       | | At the start of a run a single       |
| document          | | document is updated to the mongo     |
|                   | | collection. This contains information|
|                   | | required by the event builder to     |
|                   | | effectively trigger and record the   |
|                   | | run.                                 |
+-------------------+----------------------------------------+
