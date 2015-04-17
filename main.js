/*var toastGroupTemplate = document.querySelector('#toastGroup');
toastGroupTemplate.showToast = function() {
  document.querySelector('#toast').show();
}
*/
function OrderLine() { 
  this.product = { name: 'dus', price: 34.3 }
  this.qty = 2;
  
  this.subTotal = ko.observable(function() { 
    console.log('dus');
    return this.product ? this.product.price * this.qty : 0;
  });
  ko.observable(this); // make it observable :) 
}


function AppViewModel() { 
  this.lines = [ new OrderLine(), new OrderLine() ];
  //ko.track(this);
  ko.observable(this);
  
}


  ko.applyBindings(new AppViewModel());

