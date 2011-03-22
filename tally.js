/** utility function to return the sum of an array of values */
function ogevents_sum(values) {
  var i, sum = 0;
  for (i = 0; i < values.length; i++) {
    sum += values[i];
  }
  return sum;
}

Drupal.behaviors.ogevents = function(context) {

  // highlight on hover
  $('table.wb-inputgrid tbody tr', context).not('.sum').not('ogevents-hover-processed').addClass('ogevents-hover-processed').hover(function () {
  //$('table.wb-inputgrid tbody tr:not(.inputgrid-hover-processed)', context).addClass('inputgrid-hover-processed').not('.sum').hover(function () {
    $(this).addClass('hover');
    $(this).find('input').addClass('hover');
  }, function () {
    $(this).removeClass('hover');
    $(this).find('input').removeClass('hover');
  });

  // clear and set default value on focus and blur
  // see http://www.miuaiga.com/index.cfm/2009/8/31/jQuery-form-input-defaultValue
  $('table.wb-inputgrid tbody input:not(.ogevents-focus-processed)', context).addClass('.ogevents-focus-processed').focus(function () {
    if (this.value == $(this)[0].defaultValue || this.value == '')
      this.value='';
  });
  $('table.wb-inputgrid tbody input:not(.ogevents-blur-processed)', context).addClass('.ogevents-blur-processed').blur(function () {
    if (this.value == $(this)[0].defaultValue || this.value == '')
      this.value = $(this)[0].defaultValue;
  });

  // When any input changes, recalculate sums displayed below table
  $('#ogevents-tally-form input:text:not(.ogevents-change-processed)', context).addClass('.ogevents-change-processed').change(function () {
    ogevents_sumtable(this);
  });
  // And make sure to calculate on page/DOM load too:
  $('#ogevents-tally-form input:text:not(.ogevents-ready-processed)', context).addClass('.ogevents-ready-processed').ready(function () {
    ogevents_sumtable(this);
  });
};

function ogevents_sumtable(item) {
    var cells, value;
    // columns are assumptions about current state of html (if src changes, change this too).
    var columns = { 'walk': 2, 'bike': 3, 'other': 4 };

    if (isNaN(item.value) && item.value != undefined) {
      alert(Drupal.t("Error: This is not a number: @input", {"@input": item.value }));
      $(item).addClass('input-error').focus().select();
    }
    else if (item.value < 0) {
      alert(Drupal.t("Error: Negative numbers don't make sense: @input", {"@input": item.value }));
      $(item).addClass('input-error').focus().select();
    }
    else {
      $(item).removeClass('input-error');
    }

    $.each(columns, function (mode, colIndex) {
      cells = $('#ogevents-tally-form tr td:nth-child(' + colIndex + ') input').map(function () {
        value = (this.value == '' ? 0 : parseInt(this.value));
        return isNaN(value) ? 0 : value;
      });
      $('tr.sum td.' + mode).text(ogevents_sum(cells));
    });
}
