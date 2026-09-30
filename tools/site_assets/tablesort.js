/* Make every table sortable by clicking its heading (Material for MkDocs
   instant-navigation safe: runs again after each page change). */
document$.subscribe(function () {
  var tables = document.querySelectorAll("article table:not([class])");
  tables.forEach(function (table) { new Tablesort(table); });
  document.querySelectorAll("table.mon-all").forEach(function (table) {
    new Tablesort(table);
  });
});
