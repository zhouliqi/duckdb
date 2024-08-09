select a.address_state as g0, t1rp1 as g1, t2rp1 as g2, max(t5rp1) as p0, avg(t8rp1 * t8rp2) as p1, max(t6rp1) as p2, count(c.customer_priority) as p3, coalesce(avg(t7rp1), 0.0) as p4 from CustomerView c left outer join AddressView a on c.customer_id = a.address_customerId left outer join TaxRecordView t on a.address_id = t.taxRecord_addressId left outer join ( select sum(creditCard_cvv) as t1rp1, c.customer_id as t1pk from CustomerView c left outer join CreditCardView cc on c.customer_id = cc.creditCard_customerId group by c.customer_id ) t1 on c.customer_id = t1.t1pk left outer join ( select min(p.product_likes) as t2rp1, c.customer_id as t2pk from CustomerView c left outer join OrderView o on c.customer_id = o.order_customerId left outer join OrderItemView oi on o.order_id = oi.orderItem_orderId left outer join ProductView p on oi.orderItem_productId = p.product_id left outer join CategoryView ca on p.product_categoryName = ca.category_name where ca.category_seasonal = true group by c.customer_id ) t2 on c.customer_id = t2.t2pk left outer join ( select max(o.order_subShipments) as t5rp1, c.customer_id as t5pk from CustomerView c left outer join OrderView o on c.customer_id = o.order_customerId group by c.customer_id ) t5 on c.customer_id = t5pk left outer join ( select max(coalesce(oi.orderItem_weight, 1)) as t6rp1, c.customer_id as t6pk from CustomerView c left outer join OrderView o on c.customer_id = o.order_customerId left outer join OrderItemView oi on o.order_id = oi.orderItem_orderId where o.order_serverId in (1, 3, 5) group by c.customer_id ) t6 on c.customer_id = t6pk left outer join ( select count(ca.category_seasonal) as t7rp1, c.customer_id as t7pk from CustomerView c left outer join OrderView o on c.customer_id = o.order_customerId left outer join OrderItemView oi on o.order_id = oi.orderItem_orderId left outer join ProductView p on oi.orderItem_productId = p.product_id left outer join CategoryView ca on p.product_categoryName = ca.category_name where ca.category_perishable = true group by c.customer_id ) t7 on c.customer_id = t7pk left outer join ( select sum(creditCard_zip) as t8rp1, sum(creditCard_lastChargeAmount) as t8rp2, c.customer_id as t8pk from CustomerView c left outer join OrderView o on c.customer_id = o.order_customerId left outer join CreditCardView cc on o.order_creditCardNumber = cc.creditCard_number group by c.customer_id ) t8 on c.customer_id = t8pk where t.taxRecord_value > 149670.0 group by a.address_state, t1rp1, t2rp1 order by g0, p0, p1 limit 500;
